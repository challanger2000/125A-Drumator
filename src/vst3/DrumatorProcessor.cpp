#include "DrumatorProcessor.h"
#include "core/PatternQuantization.h"
#include "PluginIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstattributes.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <memory>

namespace drumator {
namespace {
constexpr double kPi=3.141592653589793238462643383279502884;
// Distinct, bounded processor state format (not the future pattern bank format).
constexpr Steinberg::uint32 kStateMagic=0x31524D44u; // legacy "DMR1"
constexpr Steinberg::uint32 kStateMagicV2=0x32524D44u; // "DMR2", 4096 step bytes
constexpr auto kStateMagicV3Local=kStateMagicV3; // plus visible row count
class ActiveAudioCall {
 std::atomic<std::uint32_t>& active_;
public:
 explicit ActiveAudioCall(std::atomic<std::uint32_t>& active) noexcept : active_(active) {
   active_.fetch_add(1,std::memory_order_acq_rel);
 }
 ~ActiveAudioCall() {active_.fetch_sub(1,std::memory_order_release);}
};
class KitRestoreBarrier {
 std::atomic<bool>& busy_;
public:
 KitRestoreBarrier(std::atomic<bool>& busy,
                   const std::atomic<std::uint32_t>& active) noexcept : busy_(busy) {
   busy_.store(true,std::memory_order_release);
   // Only the control thread spins. A new audio block immediately returns
   // bounded zero output while restoration is in progress.
   while(active.load(std::memory_order_acquire)!=0)
     std::this_thread::yield();
 }
 ~KitRestoreBarrier() {busy_.store(false,std::memory_order_release);}
};
}
Processor::Processor() {
 setControllerClass(kControllerCID);
 for(auto& gain:rowGain_)gain.store(1.f,std::memory_order_relaxed);
 for(auto& pan:rowPan_)pan.store(0.f,std::memory_order_relaxed);
 // The initial test pattern is supplied by AtomicPatternBank.
}
Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context) {
 const auto result=AudioEffect::initialize(context);
 if(result!=Steinberg::kResultOk)return result;
 addAudioOutput(STR16("MAIN"),Steinberg::Vst::SpeakerArr::kStereo);
 // Rows 2-16 are optional mono-instrument stereo stems, disabled by default.
 // Existing hosts using only the main bus retain their stereo drum mix.
 for(int row=1;row<static_cast<int>(kRows);++row) {
   Steinberg::Vst::String128 name{};
   const auto* prefix=STR16("DRUM ");
   for(int n=0;n<5;++n)name[n]=prefix[n];
   const int number=row+1;
   name[5]=static_cast<Steinberg::char16>('0'+number/10);
   name[6]=static_cast<Steinberg::char16>('0'+number%10);
   addAudioOutput(name,Steinberg::Vst::SpeakerArr::kStereo,
                  Steinberg::Vst::BusTypes::kAux,0);
 }
 addEventInput(STR16("MIDI Input"),16);
 processContextRequirements.needProjectTimeMusic().needTempo().needTransportState();
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::setBusArrangements(
 Steinberg::Vst::SpeakerArrangement* inputs,Steinberg::int32 numIns,
 Steinberg::Vst::SpeakerArrangement* outputs,Steinberg::int32 numOuts) {
 if(numIns!=0||numOuts<1||numOuts>static_cast<Steinberg::int32>(kRows)||
    !outputs)return Steinberg::kResultFalse;
 for(Steinberg::int32 i=0;i<numOuts;++i)
   if(outputs[i]!=Steinberg::Vst::SpeakerArr::kStereo)
      return Steinberg::kResultFalse;
 // Fixed stereo arrangements; optional stem buses are activated by host.
 return Steinberg::kResultTrue;
}
Steinberg::tresult PLUGIN_API Processor::activateBus(
 Steinberg::Vst::MediaType type,Steinberg::Vst::BusDirection dir,
 Steinberg::int32 index,Steinberg::TBool state) {
 const auto result=AudioEffect::activateBus(type,dir,index,state);
 if(result!=Steinberg::kResultOk)return result;
 if(type==Steinberg::Vst::MediaTypes::kAudio&&
    dir==Steinberg::Vst::BusDirections::kOutput&&
    index>0&&index<static_cast<Steinberg::int32>(kRows)) {
    const auto bit=std::uint32_t{1}<<static_cast<unsigned>(index);
    if(state)activeStemMask_.fetch_or(bit,std::memory_order_relaxed);
    else activeStemMask_.fetch_and(~bit,std::memory_order_relaxed);
 }
 return result;
}
Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(Steinberg::int32 size) {
 return size==Steinberg::Vst::kSample32?Steinberg::kResultTrue:Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool state) {
 if(!state) {voices_.reset();wasPlaying_=false;}
 return AudioEffect::setActive(state);
}
Steinberg::tresult PLUGIN_API Processor::setProcessing(Steinberg::TBool state) {
 // Steinberg's AudioEffect default returns kNotImplemented. A real host
 // may invoke this independently of setActive; honor the lifecycle.
 if(!state) {voices_.reset();wasPlaying_=false;}
 return Steinberg::kResultOk;
}
// Messages are delivered on the UI thread by the VST3 IConnectionPoint
// contract, never by process(). The controller will eventually send the
// bounded PCM16 WAV payload after a user picks a sample.
Steinberg::tresult PLUGIN_API Processor::notify(Steinberg::Vst::IMessage* msg) {
 if(!msg||!msg->getMessageID())return AudioEffect::notify(msg);
 const auto* id=msg->getMessageID();
 if(std::strcmp(id,"Drumator.LoadPcm16Wav")!=0&&
    std::strcmp(id,"Drumator.StepEdit")!=0&&
    std::strcmp(id,"Drumator.VisibleRows")!=0&&
    std::strcmp(id,"Drumator.RowGate")!=0&&
    std::strcmp(id,"Drumator.RowMix")!=0&&
    std::strcmp(id,"Drumator.RowPitch")!=0&&
    std::strcmp(id,"Drumator.RowChoke")!=0&&
    std::strcmp(id,"Drumator.Swing")!=0&&
    std::strcmp(id,"Drumator.Humanize")!=0&&
    std::strcmp(id,"Drumator.ClearSample")!=0)
    return AudioEffect::notify(msg);
 auto* attrs=msg->getAttributes();
 if(!attrs)return Steinberg::kInvalidArgument;
 Steinberg::int64 row=-1;
 if(std::strcmp(id,"Drumator.ClearSample")==0) {
   Steinberg::int64 index=-1;
   if(attrs->getInt("Row",index)!=Steinberg::kResultOk||
      index<0||index>=static_cast<Steinberg::int64>(kRows))
      return Steinberg::kInvalidArgument;
   const auto rate=outputRate_.load(std::memory_order_acquire);
   if(rate<=0.0||!std::isfinite(rate))return Steinberg::kResultFalse;
   std::unique_ptr<PreparedSample> silent;
   try {silent=PreparedSample::silence(rate);}
   catch(const std::bad_alloc&){return Steinberg::kOutOfMemory;}
   if(!silent)return Steinberg::kResultFalse;
   std::lock_guard<std::mutex> guard(kitMutex_);
   for(;;){auto unused=exchange_.pollReclaim();if(!unused)break;}
   auto old=exchange_.withdraw(static_cast<std::size_t>(index));
   (void)old;
   if(!exchange_.publish(static_cast<std::size_t>(index),silent))
      return Steinberg::kResultFalse;
   kit_.wav[static_cast<std::size_t>(index)].clear();
   kit_.names[static_cast<std::size_t>(index)].clear();
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.Humanize")==0) {
   Steinberg::int64 percent=-1;
   if(attrs->getInt("Percent",percent)!=Steinberg::kResultOk||
      percent<0||percent>100)return Steinberg::kInvalidArgument;
   humanizePercent_.store(static_cast<std::uint8_t>(percent),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.Swing")==0) {
   Steinberg::int64 percent=-1;
   if(attrs->getInt("Percent",percent)!=Steinberg::kResultOk||
      percent<0||percent>50)return Steinberg::kInvalidArgument;
   swingPercent_.store(static_cast<std::uint8_t>(percent),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowChoke")==0) {
   Steinberg::int64 index=-1,group=-1;
   if(attrs->getInt("Row",index)!=Steinberg::kResultOk||
      attrs->getInt("Group",group)!=Steinberg::kResultOk||
      index<0||index>=static_cast<Steinberg::int64>(kRows)||
      group<0||group>4)return Steinberg::kInvalidArgument;
   rowChoke_[static_cast<std::size_t>(index)].store(
      static_cast<std::uint8_t>(group),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowPitch")==0) {
   Steinberg::int64 rowIndex=-1,semitones=999;
   if(attrs->getInt("Row",rowIndex)!=Steinberg::kResultOk||
      attrs->getInt("Semitones",semitones)!=Steinberg::kResultOk||
      rowIndex<0||rowIndex>=static_cast<Steinberg::int64>(kRows)||
      semitones< -24||semitones>24)return Steinberg::kInvalidArgument;
   const auto idx=static_cast<std::size_t>(rowIndex);
   std::lock_guard<std::mutex> guard(kitMutex_);
   const auto rate=outputRate_.load(std::memory_order_acquire);
   if(!(rate>0.0)||!std::isfinite(rate))return Steinberg::kResultFalse;
   std::unique_ptr<PreparedSample> candidate;
   if(kit_.wav[idx].empty()) {
     try {candidate=PreparedSample::silence(rate);}
     catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
   }else if(PreparedSample::fromWav(
       kit_.wav[idx].data(),kit_.wav[idx].size(),rate,
       static_cast<double>(semitones),1.f,0.f,0,false,candidate)!=
       PrepareStatus::Ok)return Steinberg::kResultFalse;
   if(!candidate)return Steinberg::kResultFalse;
   for(;;) {auto unused=exchange_.pollReclaim();if(!unused)break;}
   auto pending=exchange_.withdraw(idx);
   (void)pending;
   if(!exchange_.publish(idx,candidate))return Steinberg::kResultFalse;
   kit_.pitch[idx]=static_cast<std::uint8_t>(semitones+24);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowMix")==0) {
   Steinberg::int64 rowIndex=-1,gainValue=-1,panValue=-1;
   if(attrs->getInt("Row",rowIndex)!=Steinberg::kResultOk||
      attrs->getInt("Gain",gainValue)!=Steinberg::kResultOk||
      attrs->getInt("Pan",panValue)!=Steinberg::kResultOk||
      rowIndex<0||rowIndex>=static_cast<Steinberg::int64>(kRows)||
      gainValue<0||gainValue>200||panValue< -100||panValue>100)
      return Steinberg::kInvalidArgument;
   rowGain_[static_cast<std::size_t>(rowIndex)].store(
       static_cast<float>(gainValue)/100.f,std::memory_order_relaxed);
   rowPan_[static_cast<std::size_t>(rowIndex)].store(
       static_cast<float>(panValue)/100.f,std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowGate")==0) {
   Steinberg::int64 isSolo=-1,isOn=-1;
   if(attrs->getInt("Row",row)!=Steinberg::kResultOk||
      attrs->getInt("Solo",isSolo)!=Steinberg::kResultOk||
      attrs->getInt("On",isOn)!=Steinberg::kResultOk||
      row<0||row>=static_cast<Steinberg::int64>(kRows)||
      (isSolo!=0&&isSolo!=1)||(isOn!=0&&isOn!=1))
     return Steinberg::kInvalidArgument;
   auto& mask=isSolo?soloMask_:muteMask_;
   const auto bit=std::uint32_t{1}<<static_cast<unsigned>(row);
   if(isOn)mask.fetch_or(bit,std::memory_order_relaxed);
   else mask.fetch_and(~bit,std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }

 if(std::strcmp(id,"Drumator.VisibleRows")==0) {
   Steinberg::int64 count=0;
   if(attrs->getInt("Count",count)!=Steinberg::kResultOk||
      !RowVisibility::valid(static_cast<std::size_t>(count)))
      return Steinberg::kInvalidArgument;
   // Only the controller hides an empty row. The processor stores the
   // visible count but intentionally retains all 16 DSP slots/steps.
   visibleRows_.store(static_cast<std::uint8_t>(count),
                      std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(msg->getMessageID(),"Drumator.StepEdit")==0) {
   Steinberg::int64 pattern=-1,step=-1,velocity=-1;
   if(attrs->getInt("Pattern",pattern)!=Steinberg::kResultOk||
      attrs->getInt("Row",row)!=Steinberg::kResultOk||
      attrs->getInt("Step",step)!=Steinberg::kResultOk||
      attrs->getInt("Velocity",velocity)!=Steinberg::kResultOk||
      pattern<0||pattern>=static_cast<Steinberg::int64>(kPatterns)||
      row<0||row>=static_cast<Steinberg::int64>(kRows)||
      step<0||step>=static_cast<Steinberg::int64>(kSteps)||
      velocity<0||velocity>127)return Steinberg::kInvalidArgument;
   return patterns_.set(static_cast<std::size_t>(pattern),
                        static_cast<std::size_t>(row),
                        static_cast<std::size_t>(step),
                        static_cast<unsigned>(velocity))
          ? Steinberg::kResultOk : Steinberg::kResultFalse;
 }
 const void* bytes=nullptr;
 Steinberg::uint32 size=0;
 constexpr Steinberg::uint32 kMaxMessageBytes=16u*1024u*1024u;
 if(attrs->getInt("Row",row)!=Steinberg::kResultOk||
    row<0||row>=static_cast<Steinberg::int64>(kRows)||
    attrs->getBinary("WavBytes",bytes,size)!=Steinberg::kResultOk||
    !bytes||size<44||size>kMaxMessageBytes)
    return Steinberg::kInvalidArgument;
 std::string fileName="SAMPLE";
 {
   const void* nameBytes=nullptr;
   Steinberg::uint32 nameLen=0;
   if(attrs->getBinary("SampleName",nameBytes,nameLen)==Steinberg::kResultOk) {
       if(!nameBytes||nameLen==0||nameLen>kMaxKitName)
           return Steinberg::kInvalidArgument;
       fileName.assign(static_cast<const char*>(nameBytes),nameLen);
   }
   if(fileName.empty()||fileName.find('/')!=std::string::npos||
      fileName.find('\\')!=std::string::npos||
      fileName.find('\0')!=std::string::npos)
       return Steinberg::kInvalidArgument;
 }
 std::vector<std::uint8_t> retained;
 try {retained.assign(static_cast<const std::uint8_t*>(bytes),
                      static_cast<const std::uint8_t*>(bytes)+size);}
 catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
 const auto rate=outputRate_.load(std::memory_order_acquire);
 if(!(rate>0.0)||!std::isfinite(rate))return Steinberg::kResultFalse;
 std::unique_ptr<PreparedSample> candidate;
 const auto prepared=PreparedSample::fromWav(
     static_cast<const std::uint8_t*>(bytes),
     static_cast<std::size_t>(size),rate,
     static_cast<double>(static_cast<int>(kit_.pitch[static_cast<std::size_t>(row)])-24),
     1.f,0.f,0,false,candidate);
 if(prepared!=PrepareStatus::Ok)return Steinberg::kResultFalse;
 {
    std::lock_guard<std::mutex> guard(kitMutex_);
    std::size_t total=size;
    for(std::size_t r=0;r<kRows;++r)
      if(r!=static_cast<std::size_t>(row)) {
         if(kit_.wav[r].size()>kMaxKitWavs-total)return Steinberg::kResultFalse;
         total+=kit_.wav[r].size();
      }
    for(;;) {auto unused=exchange_.pollReclaim();if(!unused)break;}
    // A host may open a project or request a sample before the first audio
    // callback has adopted the default empty-row marker. Replace that
    // pending marker on the CONTROL side without touching the audio thread.
    auto previousPending=exchange_.withdraw(static_cast<std::size_t>(row));
    (void)previousPending;
    if(!exchange_.publish(static_cast<std::size_t>(row),candidate))
       return Steinberg::kResultFalse;
    kit_.wav[static_cast<std::size_t>(row)].swap(retained);
    kit_.names[static_cast<std::size_t>(row)].swap(fileName);
 }
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::setupProcessing(Steinberg::Vst::ProcessSetup& setup) {
 // The VST3 host may request unusual but valid sample rates. Reject only
 // invalid values or a provably unbounded temporary fixture allocation.
 if(setup.symbolicSampleSize!=Steinberg::Vst::kSample32||
    !std::isfinite(setup.sampleRate)||setup.sampleRate<=0.0||
    setup.maxSamplesPerBlock<0)
   return Steinberg::kInvalidArgument;
 // This runs outside process() and while processing is inactive.
 const auto parent=AudioEffect::setupProcessing(setup);
 if(parent!=Steinberg::kResultTrue)return parent;
 prepared_=false;
 outputRate_.store(0.0,std::memory_order_release);
 voices_.reset(); // old voice buffers are not retained across sample-rate changes
 wasPlaying_=false;
 // A rate change invalidates any prior per-voice interpolation kernel.
 // Preserve retired asset ownership; mute existing rows until reloaded.
 for(std::size_t row=0;row<kRows;++row)voices_.setRow(row,RowSound{});
 // Deliberately silent until a real user sample was actually accepted.
 // No hidden synthesized kick to confuse failed import with working audio.
 if(!voices_.setOutputRate(setup.sampleRate))return Steinberg::kResultFalse;
 prepared_=true;
 outputRate_.store(setup.sampleRate,std::memory_order_release);
 {
    std::lock_guard<std::mutex> guard(kitMutex_);
    if(!publishRestoredKit(kit_,setup.sampleRate)) {
       outputRate_.store(0.0,std::memory_order_release);
       prepared_=false;return Steinberg::kResultFalse;
    }
 }
 return Steinberg::kResultOk;
}
Steinberg::uint32 PLUGIN_API Processor::getTailSamples() {
 // Internal sequencer can generate sound without MIDI or audio input.
 // The host must continue processing during transport; individual sample
 // duration cannot represent this generator's ongoing output.
 return Steinberg::Vst::kInfiniteTail;
}
Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data) {
 ActiveAudioCall currentCall(activeAudioCalls_);
 if(kitRestoreInProgress_.load(std::memory_order_acquire)) {
   if(data.symbolicSampleSize==Steinberg::Vst::kSample32&&
      data.numSamples>0&&data.outputs) {
     for(Steinberg::int32 bus=0;bus<data.numOutputs;++bus) {
       auto& out=data.outputs[bus];
       if(!out.channelBuffers32)continue;
       for(Steinberg::int32 ch=0;ch<out.numChannels;++ch)
         if(out.channelBuffers32[ch])
           std::fill_n(out.channelBuffers32[ch],data.numSamples,0.f);
       out.silenceFlags=3;
     }
   }
   return Steinberg::kResultOk;
 }
 if(data.numSamples<0)return Steinberg::kInvalidArgument;
 // Hosts may flush discrete automation without providing any audio buffers.
 if(data.numSamples==0||data.numOutputs==0) {
   if(data.inputParameterChanges) {
     for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
       auto* queue=data.inputParameterChanges->getParameterData(i);
       if(!queue||queue->getParameterId()!=kPatternParam)continue;
       for(Steinberg::int32 j=0;j<queue->getPointCount();++j) {
         Steinberg::int32 offset=0;
         Steinberg::Vst::ParamValue value=0;
         if(queue->getPoint(j,offset,value)==Steinberg::kResultTrue&&
            offset>=0&&std::isfinite(value))
           selected_.store(static_cast<std::uint32_t>(
               patternFromNormalized(value)),
               std::memory_order_relaxed);
       }
     }
   }
   return Steinberg::kResultOk;
 }
 if(!prepared_||data.symbolicSampleSize!=Steinberg::Vst::kSample32||
    !data.outputs||data.outputs[0].numChannels!=2)
   return Steinberg::kResultFalse;
 auto& output=data.outputs[0];
 auto* left=output.channelBuffers32 ? output.channelBuffers32[0] : nullptr;
 auto* right=output.channelBuffers32 ? output.channelBuffers32[1] : nullptr;
 // An inactive MAIN bus may legally omit audio buffers while aux buses run.
 const bool mainReady=left&&right;
 if(!mainReady&&((left!=nullptr)!=(right!=nullptr)))return Steinberg::kResultFalse;
 const auto* context=data.processContext;
 const bool playing=context&&
     ((context->state&Steinberg::Vst::ProcessContext::kPlaying)!=0);
 if(wasPlaying_&&!playing)voices_.reset(); // stop old sequencer tails once
 wasPlaying_=playing;
 // The first full block after a kit commit starts with no old-kit voices.
 const auto revision=kitRevision_.load(std::memory_order_acquire);
 if(revision!=lastAudioKitRevision_) {
   voices_.reset();
   wasPlaying_=false;
   lastAudioKitRevision_=revision;
 }
 // Adopt only complete prepared assets, without deleting on audio.
 exchange_.audioAdopt(voices_,processSetup.sampleRate);
 triggers_.clear();
 // Process program changes at their actual sample offset. Schedule each
 // constant-pattern interval separately; no allocation in the audio callback.
 if(playing&&context&&
    (context->state&(Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                     Steinberg::Vst::ProcessContext::kTempoValid))==
                    (Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                     Steinberg::Vst::ProcessContext::kTempoValid)) {
   const auto initialPpq=context->projectTimeMusic;
   const auto tempo=context->tempo;
   const auto rate=processSetup.sampleRate;
   const auto swing=swingPercent_.load(std::memory_order_relaxed)/100.0;
   const auto humanize=humanizePercent_.load(std::memory_order_relaxed)/100.0;
   std::uint32_t activePattern=selected_.load(std::memory_order_relaxed);
   std::int32_t cursor=0;
   auto scheduleRange=[&](Steinberg::int32 finish) {
     if(finish<=cursor)return;
     patterns_.select(activePattern);
     const Clock clock{initialPpq+static_cast<double>(cursor)*tempo/(60.0*rate),
                       tempo,rate,finish-cursor,true,swing,humanize};
     const auto scheduled=StepScheduler::run(patterns_,clock,[&](Trigger hit){
       hit.sampleOffset+=static_cast<std::uint32_t>(cursor);
       triggers_.appendStep(hit);
     });
     if(scheduled.status!=ScheduleStatus::Ok)triggers_.clear();
     cursor=finish;
   };
   if(data.inputParameterChanges) {
     for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
       auto* queue=data.inputParameterChanges->getParameterData(i);
       if(!queue||queue->getParameterId()!=kPatternParam)continue;
       for(Steinberg::int32 j=0;j<queue->getPointCount();++j) {
         Steinberg::int32 sampleOffset=0;
         Steinberg::Vst::ParamValue value=0;
         if(queue->getPoint(j,sampleOffset,value)!=Steinberg::kResultTrue||
            !std::isfinite(value)||sampleOffset<0||
            sampleOffset>=data.numSamples||sampleOffset<cursor)continue;
         scheduleRange(sampleOffset);
         activePattern=static_cast<std::uint32_t>(
             patternFromNormalized(value));
       }
     }
   }
   scheduleRange(data.numSamples);
   selected_.store(activePattern,std::memory_order_relaxed);
 }else {
   // Even without a valid transport clock the host may change the program.
   if(data.inputParameterChanges)
     for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
       auto* queue=data.inputParameterChanges->getParameterData(i);
       if(!queue||queue->getParameterId()!=kPatternParam)continue;
       for(Steinberg::int32 j=0;j<queue->getPointCount();++j) {
         Steinberg::int32 offset=0;Steinberg::Vst::ParamValue value=0;
         if(queue->getPoint(j,offset,value)==Steinberg::kResultTrue&&
            std::isfinite(value)&&offset>=0&&offset<data.numSamples)
           selected_.store(static_cast<std::uint32_t>(
             patternFromNormalized(value)),
             std::memory_order_relaxed);
       }
     }
 }
 // MIDI one-shot audition also works with stopped DAW transport.
 if(data.inputEvents) {
   for(Steinberg::int32 i=0;i<data.inputEvents->getEventCount();++i) {
     Steinberg::Vst::Event event{};
     if(data.inputEvents->getEvent(i,event)!=Steinberg::kResultOk||
        event.type!=Steinberg::Vst::Event::kNoteOnEvent)continue;
     triggers_.appendNoteOn(event.noteOn.pitch,event.noteOn.velocity,
                            event.sampleOffset,
                            static_cast<std::uint32_t>(data.numSamples));
   }
 }
 for(std::size_t row=0;row<kRows;++row)
   if(!voices_.setRowMix(row,rowGain_[row].load(std::memory_order_relaxed),
                           rowPan_[row].load(std::memory_order_relaxed))||
      !voices_.setRowChoke(row,rowChoke_[row].load(std::memory_order_relaxed)))
     return Steinberg::kResultFalse;
 triggers_.applyMuteSolo(muteMask_.load(std::memory_order_relaxed),
                         soloMask_.load(std::memory_order_relaxed));
 triggers_.sort();
 std::array<float*,kRows> stemsL{},stemsR{};
 for(Steinberg::int32 bus=1;bus<data.numOutputs&&
                         bus<static_cast<Steinberg::int32>(kRows);++bus) {
   const auto& stem=data.outputs[bus];
   if((activeStemMask_.load(std::memory_order_relaxed)&
       (std::uint32_t{1}<<static_cast<unsigned>(bus)))&&
      stem.numChannels==2&&stem.channelBuffers32&&
      stem.channelBuffers32[0]&&stem.channelBuffers32[1]) {
     stemsL[static_cast<std::size_t>(bus)]=stem.channelBuffers32[0];
     stemsR[static_cast<std::size_t>(bus)]=stem.channelBuffers32[1];
   }
 }
 const auto rendered=voices_.render(triggers_.data(),triggers_.size(),left,right,
                                   static_cast<std::uint32_t>(data.numSamples),
                                   stemsL.data(),stemsR.data());
 exchange_.audioCollect(voices_);
 if(rendered.status!=RenderStatus::Ok)return Steinberg::kResultFalse;
 // Conservative: do not claim silence just because a short sound ended within
 // the current block. The host can determine silence independently.
 output.silenceFlags=0;
 for(Steinberg::int32 bus=1;bus<data.numOutputs;++bus)
   data.outputs[bus].silenceFlags=0;
 return Steinberg::kResultOk;
}
// The host/control actor owns serialized sound. The audio thread never reads
// kit_ or its mutex; it only adopts immutable prepared assets at block boundaries.
bool Processor::publishRestoredKit(const KitState& kit,double rate) {
 if(!std::isfinite(rate)||rate<=0.0)return false;
 std::array<std::unique_ptr<PreparedSample>,kRows> fresh{};
 for(std::size_t row=0;row<kRows;++row) {
    if(kit.wav[row].empty()) {
       try {fresh[row]=PreparedSample::silence(rate);}
       catch(const std::bad_alloc&) {return false;}
       if(!fresh[row])return false;
    }else if(PreparedSample::fromWav(
         kit.wav[row].data(),kit.wav[row].size(),rate,
         static_cast<double>(static_cast<int>(kit.pitch[row])-24),1.f,0.f,0,false,
         fresh[row])!=PrepareStatus::Ok)return false;
 }
 for(;;) {auto unused=exchange_.pollReclaim();if(!unused)break;}
 for(std::size_t row=0;row<kRows;++row) {
    auto stale=exchange_.withdraw(row);
    (void)stale;
 }
 for(std::size_t row=0;row<kRows;++row)
    if(!exchange_.publish(row,fresh[row]))return false;
 return true;
}
Steinberg::tresult PLUGIN_API Processor::getState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 std::vector<std::uint8_t> serialized;
 {
   std::lock_guard<std::mutex> guard(kitMutex_);
   kit_.selected=static_cast<std::uint8_t>(selected_.load(std::memory_order_relaxed));
   kit_.visible=visibleRows_.load(std::memory_order_relaxed);
   kit_.muted=static_cast<std::uint16_t>(muteMask_.load(std::memory_order_relaxed));
   kit_.solo=static_cast<std::uint16_t>(soloMask_.load(std::memory_order_relaxed));
   kit_.swing=swingPercent_.load(std::memory_order_relaxed);
   kit_.humanize=humanizePercent_.load(std::memory_order_relaxed);
   for(std::size_t row=0;row<kRows;++row)kit_.choke[row]=rowChoke_[row].load();
   for(std::size_t row=0;row<kRows;++row) {
     kit_.gain[row]=static_cast<std::uint8_t>(std::lround(rowGain_[row].load()*100.f));
     kit_.pan[row]=static_cast<std::uint8_t>(std::lround(rowPan_[row].load()*100.f+100.f));
   }
   for(std::size_t p=0;p<kPatterns;++p)
    for(std::size_t r=0;r<kRows;++r)
     for(std::size_t s=0;s<kSteps;++s)
       kit_.grid[AtomicPatternBank::index(p,r,s)]=patterns_.get(p,r,s);
   if(!KitCodec::encode(kit_,serialized))return Steinberg::kResultFalse;
 }
 const auto length=static_cast<Steinberg::TSize>(serialized.size());
 return stream.writeRaw(serialized.data(),length)==length
        ? Steinberg::kResultOk : Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Processor::setState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 Steinberg::uint32 magic=0,next=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(next))
    return Steinberg::kResultFalse;
 KitState candidate;
 if(magic==kKitMagic||magic==kKitMagicV9||magic==kKitMagicV8||magic==kKitMagicV7||magic==kKitMagicV6||magic==kKitMagicV5||magic==kKitMagicV4) {
    if(next<2+kCells+kRows*6+4||next>kMaxKitBytes-8)
       return Steinberg::kResultFalse;
    try {
       std::vector<std::uint8_t> encoded(8+static_cast<std::size_t>(next));
       for(int i=0;i<4;++i) {
           encoded[i]=static_cast<std::uint8_t>(magic>>(8*i));
           encoded[4+i]=static_cast<std::uint8_t>(next>>(8*i));
       }
       if(stream.readRaw(encoded.data()+8,next)!=static_cast<Steinberg::TSize>(next)||
          !KitCodec::decode(encoded.data(),encoded.size(),candidate))
          return Steinberg::kResultFalse;
    }catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
 }else {
    if(next>=kPatterns)return Steinberg::kResultFalse;
    candidate.selected=static_cast<std::uint8_t>(next);
    candidate.visible=static_cast<std::uint8_t>(kRows);
    if(magic==kStateMagic) {
       AtomicPatternBank defaults;
       for(std::size_t p=0;p<kPatterns;++p)
        for(std::size_t r=0;r<kRows;++r)
         for(std::size_t s=0;s<kSteps;++s)
          candidate.grid[AtomicPatternBank::index(p,r,s)]=defaults.get(p,r,s);
    }else if(magic==kStateMagicV2||magic==kStateMagicV3Local) {
       if(magic==kStateMagicV3Local &&
          (!stream.readInt8u(candidate.visible)||
            !RowVisibility::valid(candidate.visible)))
          return Steinberg::kResultFalse;
       for(auto& cell:candidate.grid)
          if(!stream.readInt8u(cell)||cell>127)
             return Steinberg::kResultFalse;
    }else return Steinberg::kResultFalse;
 }
 const auto rate=outputRate_.load(std::memory_order_acquire);
 {
    std::lock_guard<std::mutex> guard(kitMutex_);
    KitRestoreBarrier restoreGate(kitRestoreInProgress_,activeAudioCalls_);
    if(rate>0.0&&!publishRestoredKit(candidate,rate))
       return Steinberg::kResultFalse;
    kit_=std::move(candidate);
    for(std::size_t p=0;p<kPatterns;++p)
     for(std::size_t r=0;r<kRows;++r)
      for(std::size_t s=0;s<kSteps;++s)
       patterns_.set(p,r,s,
          kit_.grid[AtomicPatternBank::index(p,r,s)]);
    selected_.store(kit_.selected,std::memory_order_relaxed);
    patterns_.select(kit_.selected);
    visibleRows_.store(kit_.visible,std::memory_order_relaxed);
    muteMask_.store(kit_.muted,std::memory_order_relaxed);
    soloMask_.store(kit_.solo,std::memory_order_relaxed);
    swingPercent_.store(kit_.swing,std::memory_order_relaxed);
    humanizePercent_.store(kit_.humanize,std::memory_order_relaxed);
    for(std::size_t row=0;row<kRows;++row)rowChoke_[row].store(kit_.choke[row]);
    for(std::size_t row=0;row<kRows;++row) {
      rowGain_[row].store(kit_.gain[row]/100.f,std::memory_order_relaxed);
      rowPan_[row].store((static_cast<int>(kit_.pan[row])-100)/100.f,std::memory_order_relaxed);
    }
    kitRevision_.fetch_add(1,std::memory_order_release);
 }
 return Steinberg::kResultOk;
}
}
