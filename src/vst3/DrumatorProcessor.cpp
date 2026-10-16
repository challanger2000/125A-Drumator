#include "DrumatorProcessor.h"
#include "PluginIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <algorithm>
#include <cmath>
#include <limits>

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
 if(!state)voices_.reset();
 return AudioEffect::setActive(state);
}
Steinberg::tresult PLUGIN_API Processor::setupProcessing(Steinberg::Vst::ProcessSetup& setup) {
 if(setup.symbolicSampleSize!=Steinberg::Vst::kSample32||
    !std::isfinite(setup.sampleRate)||setup.sampleRate<8000.0||
    setup.sampleRate>192000.0||setup.maxSamplesPerBlock<0)
   return Steinberg::kInvalidArgument;
 // This runs outside process() and while processing is inactive.
 const auto parent=AudioEffect::setupProcessing(setup);
 if(parent!=Steinberg::kResultTrue)return parent;
 prepared_=false;
 voices_.reset(); // old voice buffers are not retained across sample-rate changes
 const std::size_t length=static_cast<std::size_t>(
    std::ceil(setup.sampleRate*0.35));
 testSound_.assign(length,0.f);
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
 const bool playing=context&&((context->state&Steinberg::Vst::ProcessContext::kPlaying)!=0);
 if(!playing) {
   voices_.reset();
   std::fill_n(left,data.numSamples,0.f);
   std::fill_n(right,data.numSamples,0.f);
   output.silenceFlags=3;
   return Steinberg::kResultOk;
 }
 patterns_.select(selected_.load(std::memory_order_relaxed));
 std::size_t count=0;
 // Missing/invalid clock: no new steps, do not invent a timing fallback.
 if((context->state & (Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                      Steinberg::Vst::ProcessContext::kTempoValid))==
                     (Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                      Steinberg::Vst::ProcessContext::kTempoValid)) {
   const Clock clock{context->projectTimeMusic,context->tempo,
                     processSetup.sampleRate,data.numSamples,true};
   const auto scheduled=StepScheduler::run(patterns_,clock,[&](Trigger t){
       if(count<events_.size())events_[count++]=t;
   });
   if(scheduled.status!=ScheduleStatus::Ok)count=0;
 }
 const auto rendered=voices_.render(events_.data(),count,left,right,
                                   static_cast<std::uint32_t>(data.numSamples));
 if(rendered.status!=RenderStatus::Ok)return Steinberg::kResultFalse;
 // Conservative: do not claim silence just because a short sound ended within
 // the current block. The host can determine silence independently.
 output.silenceFlags=0;
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::getState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,Steinberg::kLittleEndian);
 return stream.writeInt32u(kStateMagic)&&
        stream.writeInt32u(selected_.load(std::memory_order_relaxed))
        ? Steinberg::kResultOk : Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Processor::setState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,Steinberg::kLittleEndian);
 Steinberg::uint32 magic=0,pattern=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(pattern)||
    magic!=kStateMagic||pattern>=kPatterns)
   return Steinberg::kResultFalse;
 selected_.store(pattern,std::memory_order_relaxed);
 return Steinberg::kResultOk;
}
}
