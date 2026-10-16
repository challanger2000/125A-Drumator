#pragma once
#include "core/SequencerCore.h"
#include "core/AtomicPatternBank.h"
#include "core/VisibleRows.h"
#include "core/SampleVoiceEngine.h"
#include "core/SampleExchange.h"
#include "core/TriggerBatch.h"
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
 Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement*,
     Steinberg::int32,Steinberg::Vst::SpeakerArrangement*,Steinberg::int32) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::uint32 PLUGIN_API getTailSamples() SMTG_OVERRIDE;
private:
 AtomicPatternBank patterns_;
 SampleVoiceEngine voices_;

 SampleExchange exchange_{};
 TriggerBatch triggers_{};
 std::atomic<double> outputRate_{0.0};
 bool wasPlaying_{false};
 std::atomic<std::uint32_t> selected_{0};
 std::atomic<std::uint8_t> visibleRows_{kDefaultVisibleRows};
 bool prepared_{false};
};
}
