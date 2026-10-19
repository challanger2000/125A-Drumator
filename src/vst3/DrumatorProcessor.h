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
// Atomic, versioned transfer; only the control thread writes. The audio thread
// reads bounded snapshots and keeps its last valid chain on contention.
class SongMailbox final {
 std::atomic<std::uint32_t> sequence_{0};
 std::atomic<std::uint8_t> enabled_{0};
 std::atomic<std::uint8_t> count_{8};
 std::array<std::atomic<std::uint8_t>,SongChain::kCapacity> pattern_{};
 std::array<std::atomic<std::uint8_t>,SongChain::kCapacity> length_{};
public:
 SongMailbox() noexcept {
  for(std::size_t i=0;i<SongChain::kCapacity;++i){
   pattern_[i].store(static_cast<std::uint8_t>(i<kPatterns?i:0));
   length_[i].store(32);
  }
 }
 void publish(const SongChain& chain,bool enabled) noexcept {
  sequence_.fetch_add(1,std::memory_order_acq_rel);
  enabled_.store(enabled?1:0,std::memory_order_relaxed);
  count_.store(static_cast<std::uint8_t>(chain.count()),std::memory_order_relaxed);
  for(std::size_t i=0;i<SongChain::kCapacity;++i){
   const auto e=chain.entry(i);
   pattern_[i].store(e.pattern,std::memory_order_relaxed);
   length_[i].store(e.steps,std::memory_order_relaxed);
  }
  sequence_.fetch_add(1,std::memory_order_release);
 }
 bool snapshot(SongChain& chain,bool& enabled) const noexcept {
  const auto first=sequence_.load(std::memory_order_acquire);
  if(first&1u)return false;
  const auto count=count_.load(std::memory_order_relaxed);
  const auto state=enabled_.load(std::memory_order_relaxed);
  SongChain candidate;
  if(!candidate.setCount(count)||state>1)return false;
  for(std::size_t i=0;i<count;++i)
   if(!candidate.configure(i,pattern_[i].load(std::memory_order_relaxed),
                              length_[i].load(std::memory_order_relaxed)))return false;
  const auto last=sequence_.load(std::memory_order_acquire);
  if(first!=last||last&1u)return false;
  chain=candidate;enabled=state!=0;return true;
 }
};
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
 SongMailbox songMailbox_{};
 SongChain audioSongChain_{};
 bool audioSongEnabled_{false};
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
 std::array<std::atomic<std::uint8_t>,kRows> randomVelocity_{};
 std::atomic<std::uint32_t> activeStemMask_{0};
 bool prepared_{false};
};
}
