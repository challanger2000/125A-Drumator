#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "pluginterfaces/vst/ivstaudioprocessor.h"
namespace drumator {
// The processor is the sole audio-thread writer. The editor periodically
// exchanges block peaks; clip bits are sticky until explicitly cleared.
// All registry allocations/locks run outside process().
class OutputMeter final {
public:
 static constexpr unsigned kBuses=16;
 std::atomic<std::uint32_t> mainL{0},mainR{0};
 std::atomic<std::uint32_t> clipMask{0},fullScaleMask{0};
 // One coherent L/R block snapshot for control-thread IPC, independent of
 // destructive legacy test probes and of the number of editor windows.
 std::atomic<std::uint64_t> lastPair{0},blockSequence{0};
 std::atomic<std::uint32_t> clipResetEpoch{0};
 static std::uint32_t encode(float peak) noexcept {
   return static_cast<std::uint32_t>(std::clamp(static_cast<double>(peak),0.,4.)*16383.75+0.5);
 }
 static float decode(std::uint32_t encoded) noexcept {
   return static_cast<float>(encoded)/16383.75f;
 }
 static void accumulate(std::atomic<std::uint32_t>& slot,std::uint32_t value) noexcept {
   auto old=slot.load(std::memory_order_relaxed);
   while(old<value&&!slot.compare_exchange_weak(
       old,value,std::memory_order_relaxed,std::memory_order_relaxed)){}
 }
 void observe(const Steinberg::Vst::ProcessData& data,std::uint32_t activeStems) noexcept {
   if(data.numSamples<=0||!data.outputs)return;
   std::uint32_t encodedL=0,encodedR=0;
   for(Steinberg::int32 b=0;b<data.numOutputs&&b<16;++b){
     if(b>0&&(activeStems&(1u<<b))==0)continue;
     const auto& bus=data.outputs[b];
     if(bus.numChannels!=2||!bus.channelBuffers32||
        !bus.channelBuffers32[0]||!bus.channelBuffers32[1])continue;
     float peakL=0.f,peakR=0.f;
     bool clip=false,full=false;
     for(Steinberg::int32 i=0;i<data.numSamples;++i){
       const float l=std::fabs(bus.channelBuffers32[0][i]);
       const float r=std::fabs(bus.channelBuffers32[1][i]);
       if(!std::isfinite(l)||!std::isfinite(r)){clip=true;full=true;continue;}
       peakL=std::max(peakL,l);peakR=std::max(peakR,r);
       clip|=(l>1.f||r>1.f);
       full|=(l>=1.f||r>=1.f);
     }
     if(b==0){
       encodedL=encode(peakL);encodedR=encode(peakR);
       // Single audio writer; no UI reader ever writes to these in production.
       mainL.store(encodedL,std::memory_order_relaxed);
       mainR.store(encodedR,std::memory_order_relaxed);
     }
     if(clip)clipMask.fetch_or(1u<<b,std::memory_order_relaxed);
     if(full)fullScaleMask.fetch_or(1u<<b,std::memory_order_relaxed);
   }
   lastPair.store((static_cast<std::uint64_t>(encodedL)<<32)|encodedR,
                  std::memory_order_release);
   blockSequence.fetch_add(1,std::memory_order_release);
 }
 void clearClips() noexcept {
   clipMask.exchange(0,std::memory_order_acq_rel);
   fullScaleMask.exchange(0,std::memory_order_acq_rel);
   clipResetEpoch.fetch_add(1,std::memory_order_release);
 }
};
inline std::atomic<std::uint64_t> nextMeterId{1};
inline std::mutex meterRegistryMutex;
inline std::unordered_map<std::uint64_t,std::weak_ptr<OutputMeter>> meterRegistry;
inline std::shared_ptr<OutputMeter> findMeter(std::uint64_t key) {
 std::lock_guard<std::mutex> guard(meterRegistryMutex);
 const auto it=meterRegistry.find(key);
 return it==meterRegistry.end()?nullptr:it->second.lock();
}
inline void registerMeter(std::uint64_t key,const std::shared_ptr<OutputMeter>& meter){
 std::lock_guard<std::mutex> guard(meterRegistryMutex);meterRegistry[key]=meter;
}
inline void unregisterMeter(std::uint64_t key){
 std::lock_guard<std::mutex> guard(meterRegistryMutex);meterRegistry.erase(key);
}
}
