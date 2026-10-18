#pragma once
#include "core/SequencerCore.h"
#include "core/AtomicPatternBank.h"
#include "core/VisibleRows.h"
#include "core/KitState.h"
#include "core/SampleVoiceEngine.h"
#include "core/SampleExchange.h"
#include "core/TriggerBatch.h"
#include "core/KitRestoreGate.h"
#include "core/AtomicPatternSelection.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>
#include <memory>
namespace drumator {
class Processor final:public Steinberg::Vst::AudioEffect {
public:
 Processor();
 static Steinberg::FUnknown* createInstance(void*) {
   return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor());
 }
 Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement*,
     Steinberg::int32,Steinberg::Vst::SpeakerArrangement*,Steinberg::int32) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API activateBus(Steinberg::Vst::MediaType,
     Steinberg::Vst::BusDirection,Steinberg::int32,Steinberg::TBool) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::uint32 PLUGIN_API getTailSamples() SMTG_OVERRIDE;
private:
 bool publishRestoredKit(const KitState& state,double rate); // control only
 std::mutex kitMutex_{};
 // Restore barrier: no audio callback may consume a partially installed kit.
 // The audio callback never waits; only control-side setState waits for a block.
 KitRestoreGate restoreGate_{};
 std::atomic<std::uint32_t> kitRevision_{0};
 // Version and selected program share one atomic CAS arbitration word.
 std::uint32_t lastAudioKitRevision_{0}; // audio-thread only
 KitState kit_{};
 AtomicPatternBank patterns_;
 // Last coherent patterns, read/written only in the audio callback.
 std::array<std::array<std::uint8_t,kRows*kSteps>,kPatterns> audioPatternCache_{};
 std::array<bool,kPatterns> audioPatternValid_{};
 SampleVoiceEngine voices_;

 SampleExchange exchange_{};
 TriggerBatch triggers_{};
 std::atomic<double> outputRate_{0.0};
 bool wasPlaying_{false};
 std::int64_t phaseOriginStep_{0};
 double expectedNextPpq_{0.0};
 bool haveExpectedPpq_{false};
 AtomicPatternSelection selected_{};
 std::atomic<std::uint8_t> visibleRows_{kDefaultVisibleRows};
 std::atomic<std::uint32_t> muteMask_{0};
 std::atomic<std::uint32_t> soloMask_{0};
 std::array<std::atomic<float>,kRows> rowGain_{};
 std::array<std::atomic<float>,kRows> rowPan_{};
 std::array<std::atomic<std::uint8_t>,kRows> rowChoke_{};
 std::array<std::atomic<std::uint8_t>,kRows> rowOutput_{};
 std::atomic<std::uint8_t> swingPercent_{0};
 std::atomic<std::uint8_t> humanizePercent_{0};
 std::atomic<std::uint32_t> activeStemMask_{0};
 bool prepared_{false};
};
}
