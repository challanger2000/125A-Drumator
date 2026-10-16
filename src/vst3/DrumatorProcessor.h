#pragma once
#include "core/SequencerCore.h"
#include "core/SampleVoiceEngine.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
namespace drumator {
class Processor final:public Steinberg::Vst::AudioEffect {
public:
 Processor();
 static Steinberg::FUnknown* createInstance(void*) {
   return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor());
 }
 Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement*,
     Steinberg::int32,Steinberg::Vst::SpeakerArrangement*,Steinberg::int32) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::uint32 PLUGIN_API getTailSamples() SMTG_OVERRIDE;
private:
 PatternBank patterns_;
 SampleVoiceEngine voices_;
 SincKernel kernel_;
 std::vector<float> testSound_; // populated ONLY in setupProcessing, never in process()
 std::array<Trigger,kRows*StepScheduler::maxBoundariesPerBlock> events_{};
 std::atomic<std::uint32_t> selected_{0};
 bool prepared_{false};
};
}
