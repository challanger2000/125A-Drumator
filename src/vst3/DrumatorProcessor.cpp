#include "DrumatorProcessor.h"
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

namespace drumator {
namespace {
constexpr double kPi=3.141592653589793238462643383279502884;
// Distinct, bounded processor state format (not the future pattern bank format).
constexpr Steinberg::uint32 kStateMagic=0x31524D44u; // legacy "DMR1"
constexpr Steinberg::uint32 kStateMagicV2=0x32524D44u; // "DMR2", 4096 step bytes
constexpr auto kStateMagicV3Local=kStateMagicV3; // plus visible row count
}
Processor::Processor() {
 setControllerClass(kControllerCID);
 // The initial test pattern is supplied by AtomicPatternBank.
}
Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context) {
 const auto result=AudioEffect::initialize(context);
 if(result!=Steinberg::kResultOk)return result;
 addAudioOutput(STR16("MAIN"),Steinberg::Vst::SpeakerArr::kStereo);
 addEventInput(STR16("MIDI Input"),16);
 processContextRequirements.needProjectTimeMusic().needTempo().needTransportState();
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::setBusArrangements(
 Steinberg::Vst::SpeakerArrangement* inputs,Steinberg::int32 numIns,
 Steinberg::Vst::SpeakerArrangement* outputs,Steinberg::int32 numOuts) {
 if(numIns!=0||numOuts!=1||!outputs||
    outputs[0]!=Steinberg::Vst::SpeakerArr::kStereo)return Steinberg::kResultFalse;
 return AudioEffect::setBusArrangements(inputs,numIns,outputs,numOuts);
}
Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(Steinberg::int32 size) {
 return size==Steinberg::Vst::kSample32?Steinberg::kResultTrue:Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool state) {
 if(!state) {voices_.reset();wasPlaying_=false;}
 return AudioEffect::setActive(state);
}
// Messages are delivered on the UI thread by the VST3 IConnectionPoint
// contract, never by process(). The controller will eventually send the
// bounded PCM16 WAV payload after a user picks a sample.
Steinberg::tresult PLUGIN_API Processor::notify(Steinberg::Vst::IMessage* msg) {
 if(!msg||!msg->getMessageID())return AudioEffect::notify(msg);
 const auto* id=msg->getMessageID();
 if(std::strcmp(id,"Drumator.LoadPcm16Wav")!=0&&
    std::strcmp(id,"Drumator.StepEdit")!=0&&
    std::strcmp(id,"Drumator.VisibleRows")!=0)
    return AudioEffect::notify(msg);
 auto* attrs=msg->getAttributes();
 if(!attrs)return Steinberg::kInvalidArgument;
 Steinberg::int64 row=-1;
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
 // The SPSC return queue's control-side consumer runs on the same
 // connection/UI thread that publishes the next sound.
 for(;;) {
   auto unused=exchange_.pollReclaim();
   if(!unused)break;
 }
 const auto rate=outputRate_.load(std::memory_order_acquire);
 if(!(rate>0.0)||!std::isfinite(rate))return Steinberg::kResultFalse;
 std::unique_ptr<PreparedSample> candidate;
 const auto prepared=PreparedSample::fromWav(
     static_cast<const std::uint8_t*>(bytes),
     static_cast<std::size_t>(size),rate,0.0,1.f,0.f,0,false,candidate);
 if(prepared!=PrepareStatus::Ok)return Steinberg::kResultFalse;
 if(!exchange_.publish(static_cast<std::size_t>(row),candidate))
    return Steinberg::kResultFalse; // previous pending sound is preserved
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
 return Steinberg::kResultOk;
}
Steinberg::uint32 PLUGIN_API Processor::getTailSamples() {
 return 0; // no internal test-sound fixture; sample voice tails vary by file
}
Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data) {
 // A single discrete pattern parameter is implemented at buffer granularity
 // during this integration milestone; accurate in-block program changes pending.
 if(data.inputParameterChanges) {
   for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
     auto* q=data.inputParameterChanges->getParameterData(i);
     if(!q||q->getParameterId()!=kPatternParam||q->getPointCount()<=0)continue;
     Steinberg::int32 sampleOffset=0;
     Steinberg::Vst::ParamValue value=0;
     if(q->getPoint(q->getPointCount()-1,sampleOffset,value)==Steinberg::kResultTrue&&
        std::isfinite(value)) {
       const auto clamped=std::clamp(value,0.0,1.0);
       selected_.store(static_cast<std::uint32_t>(std::lround(clamped*7.0)),
                       std::memory_order_relaxed);
     }
   }
 }
 if(data.numSamples<0)return Steinberg::kInvalidArgument;
 if(data.numSamples==0||data.numOutputs==0)return Steinberg::kResultOk;
 if(!prepared_||data.symbolicSampleSize!=Steinberg::Vst::kSample32||
    !data.outputs||data.outputs[0].numChannels!=2||
    !data.outputs[0].channelBuffers32||
    !data.outputs[0].channelBuffers32[0]||
    !data.outputs[0].channelBuffers32[1])
   return Steinberg::kResultFalse;
 auto& output=data.outputs[0];
 auto* left=output.channelBuffers32[0];
 auto* right=output.channelBuffers32[1];
 const auto* context=data.processContext;
 const bool playing=context&&
     ((context->state&Steinberg::Vst::ProcessContext::kPlaying)!=0);
 if(wasPlaying_&&!playing)voices_.reset(); // stop old sequencer tails once
 wasPlaying_=playing;
 // Adopt only complete prepared assets, without deleting on audio.
 exchange_.audioAdopt(voices_,processSetup.sampleRate);
 triggers_.clear();
 if(playing&&context) {
   patterns_.select(selected_.load(std::memory_order_relaxed));
   // Missing/invalid clock: no new steps, no invented fallback.
   if((context->state & (Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                        Steinberg::Vst::ProcessContext::kTempoValid))==
                       (Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                        Steinberg::Vst::ProcessContext::kTempoValid)) {
     const Clock clock{context->projectTimeMusic,context->tempo,
                       processSetup.sampleRate,data.numSamples,true};
     const auto scheduled=StepScheduler::run(patterns_,clock,[&](Trigger t){
         triggers_.appendStep(t);
     });
     if(scheduled.status!=ScheduleStatus::Ok)triggers_.clear();
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
 triggers_.sort();
 const auto rendered=voices_.render(triggers_.data(),triggers_.size(),left,right,
                                   static_cast<std::uint32_t>(data.numSamples));
 exchange_.audioCollect(voices_);
 if(rendered.status!=RenderStatus::Ok)return Steinberg::kResultFalse;
 // Conservative: do not claim silence just because a short sound ended within
 // the current block. The host can determine silence independently.
 output.silenceFlags=0;
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::getState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 // UI edits use atomic cells. Snapshot is safe against data races.
 const auto stored=patterns_.snapshot();
 if(!stream.writeInt32u(kStateMagicV3Local)||
    !stream.writeInt32u(selected_.load(std::memory_order_relaxed))||
    !stream.writeInt8u(visibleRows_.load(std::memory_order_relaxed)))
     return Steinberg::kResultFalse;
 for(std::size_t i=6;i<stored.size();++i)
     if(!stream.writeInt8u(stored[i]))return Steinberg::kResultFalse;
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::setState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 Steinberg::uint32 magic=0,pattern=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(pattern)||pattern>=kPatterns)
     return Steinberg::kResultFalse;
 if(magic==kStateMagic) {
     // Legacy projects were created with all 16 rows visible.
     selected_.store(pattern,std::memory_order_relaxed);
     patterns_.select(pattern);
     visibleRows_.store(static_cast<std::uint8_t>(kRows),std::memory_order_relaxed);
     return Steinberg::kResultOk;
 }
 if(magic!=kStateMagicV2&&magic!=kStateMagicV3Local)
    return Steinberg::kResultFalse;
 std::uint8_t rows=static_cast<std::uint8_t>(kRows);
 if(magic==kStateMagicV3Local&&
    (!stream.readInt8u(rows)||!RowVisibility::valid(rows)))
     return Steinberg::kResultFalse;
 std::array<std::uint8_t,kCells> cells{};
 for(auto& cell:cells) {
     if(!stream.readInt8u(cell)||cell>127)return Steinberg::kResultFalse;
 }
 // Validate the complete state BEFORE changing live atomic cells.
 for(std::size_t p=0;p<kPatterns;++p)
   for(std::size_t r=0;r<kRows;++r)
     for(std::size_t s=0;s<kSteps;++s)
       patterns_.set(p,r,s,cells[AtomicPatternBank::index(p,r,s)]);
 selected_.store(pattern,std::memory_order_relaxed);
 patterns_.select(pattern);
 visibleRows_.store(rows,std::memory_order_relaxed);
 return Steinberg::kResultOk;
}
}
