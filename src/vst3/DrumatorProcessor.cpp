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
constexpr Steinberg::uint32 kStateMagic=0x31524D44u; // "DMR1", little endian
}
Processor::Processor() {
 setControllerClass(kControllerCID);
 // Temporary deterministic test pattern, not factory Drumator content.
 for(std::size_t p=0;p<kPatterns;++p)
   for(std::size_t step=0;step<kSteps;step+=4)
     patterns_.set(p,0,step,step%8==0?127:90);
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
 if(!msg||!msg->getMessageID()||
    std::strcmp(msg->getMessageID(),"Drumator.LoadPcm16Wav")!=0)
    return AudioEffect::notify(msg);
 auto* attrs=msg->getAttributes();
 if(!attrs)return Steinberg::kInvalidArgument;
 Steinberg::int64 row=-1;
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
 constexpr double kTestSoundSeconds=0.35;
 constexpr std::size_t kMaxPreparedTestFrames=1048576; // 4 MiB of mono float PCM
 if(setup.symbolicSampleSize!=Steinberg::Vst::kSample32||
    !std::isfinite(setup.sampleRate)||setup.sampleRate<=0.0||
    setup.maxSamplesPerBlock<0)
   return Steinberg::kInvalidArgument;
 const double requestedFrames=std::ceil(setup.sampleRate*kTestSoundSeconds);
 if(!std::isfinite(requestedFrames)||requestedFrames<1.0||
    requestedFrames>static_cast<double>(kMaxPreparedTestFrames))
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
 const std::size_t length=static_cast<std::size_t>(requestedFrames);
 try {
   testSound_.assign(length,0.f); // setupProcessing only, never realtime
 } catch(const std::bad_alloc&) {
   testSound_.clear();
   return Steinberg::kResultFalse;
 }
 for(std::size_t i=0;i<length;++i) {
   const double time=static_cast<double>(i)/setup.sampleRate;
   const double f=60.0*std::exp(-time*12.0)+55.0;
   // Temporary synthesized kick for integration checks ONLY.
   testSound_[i]=static_cast<float>(
     0.35*std::sin(2.0*kPi*f*time)*std::exp(-time*25.0));
 }
 if(!kernel_.prepare(1.0)||!voices_.setOutputRate(setup.sampleRate))
   return Steinberg::kResultFalse;
 RowSound row;
 row.sample={testSound_.data(),nullptr,testSound_.size()};
 row.kernel=&kernel_;
 row.gain=0.8f;
 if(!voices_.setRow(0,row))return Steinberg::kResultFalse;
 prepared_=true;
 outputRate_.store(setup.sampleRate,std::memory_order_release);
 return Steinberg::kResultOk;
}
Steinberg::uint32 PLUGIN_API Processor::getTailSamples() {
 return static_cast<Steinberg::uint32>(testSound_.size());
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
 return stream.writeInt32u(kStateMagic)&&
        stream.writeInt32u(selected_.load(std::memory_order_relaxed))
        ? Steinberg::kResultOk : Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Processor::setState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 Steinberg::uint32 magic=0,pattern=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(pattern)||
    magic!=kStateMagic||pattern>=kPatterns)
   return Steinberg::kResultFalse;
 selected_.store(pattern,std::memory_order_relaxed);
 return Steinberg::kResultOk;
}
}
