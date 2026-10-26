// 125A Drumator - single-step atomic bank for UI->audio control.
// Every cell is independently atomic; no callback allocation, file I/O,
// blocking locks, or race on the audio-thread scheduler.
#pragma once
#include "SequencerCore.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace drumator {
class AtomicPatternBank final {
public:
    AtomicPatternBank() noexcept {
        // Current VST3 technical prototype begins with a test rhythm.
        for(auto& length:lengths_)length.store(32,std::memory_order_relaxed);
        // Only A contains the factory kick; B-H (including steps 17-32)
        // remain empty for immediate non-destructive DUPLICATE.
        for(std::size_t s=0;s<kSteps;s+=4)
            set(0,0,s,(s%8==0)?127:90);
    }
    bool set(std::size_t p,std::size_t row,std::size_t step,
             unsigned velocity) noexcept {
        if(p>=kPatterns||row>=kRows||step>=kSteps||velocity>127)
            return false;
        std::lock_guard<std::mutex> guard(writeMutex_);
        version_[p].fetch_add(1,std::memory_order_acq_rel);
        grid_[index(p,row,step)].store(static_cast<std::uint8_t>(velocity),
                                       std::memory_order_relaxed);
        version_[p].fetch_add(1,std::memory_order_release);
        return true;
    }
    std::uint8_t get(std::size_t p,std::size_t row,std::size_t step) const noexcept {
        if(p>=kPatterns||row>=kRows||step>=kSteps)return 0;
        return grid_[index(p,row,step)].load(std::memory_order_relaxed);
    }
    // Obtain a complete generation, or reject the attempt. Audio never waits.
    bool coherentPattern(std::size_t p,
                         std::array<std::uint8_t,kRows*kSteps>& out,
                         std::uint8_t& duration) const noexcept {
        if(p>=kPatterns)return false;
        const auto before=version_[p].load(std::memory_order_acquire);
        if(before&1u)return false;
        const auto candidateLength=lengths_[p].load(std::memory_order_relaxed);
        std::array<std::uint8_t,kRows*kSteps> candidate{};
        for(std::size_t i=0;i<candidate.size();++i)
            candidate[i]=grid_[p*kRows*kSteps+i].load(std::memory_order_relaxed);
        const auto after=version_[p].load(std::memory_order_acquire);
        if(before!=after|| (after&1u))return false;
        if(candidateLength!=16&&candidateLength!=32)return false;
        out=candidate;duration=candidateLength;
        return true;
    }
    bool coherentPattern(std::size_t p,
                         std::array<std::uint8_t,kRows*kSteps>& out) const noexcept {
        std::uint8_t ignored=32;
        return coherentPattern(p,out,ignored);
    }
    bool setLength(std::size_t p,std::uint8_t duration) noexcept {
        if(p>=kPatterns||(duration!=16&&duration!=32))return false;
        std::lock_guard<std::mutex> guard(writeMutex_);
        version_[p].fetch_add(1,std::memory_order_acq_rel);
        lengths_[p].store(duration,std::memory_order_relaxed);
        version_[p].fetch_add(1,std::memory_order_release);
        return true;
    }
    bool replacePattern(std::size_t p,const std::uint8_t* values,std::size_t count,
                        std::uint8_t duration=0) noexcept {
        if(p>=kPatterns||!values||count!=kRows*kSteps||
           (duration!=0&&duration!=16&&duration!=32))return false;
        for(std::size_t i=0;i<count;++i)if(values[i]>127)return false;
        std::lock_guard<std::mutex> guard(writeMutex_);
        version_[p].fetch_add(1,std::memory_order_acq_rel);
        for(std::size_t i=0;i<count;++i)
            grid_[p*kRows*kSteps+i].store(values[i],std::memory_order_relaxed);
        if(duration!=0)lengths_[p].store(duration,std::memory_order_relaxed);
        version_[p].fetch_add(1,std::memory_order_release);
        return true;
    }
    // Host/control-thread project export: lock the same writer mutex as
    // set() and replacePattern(), so a saved kit cannot contain half of
    // an in-flight full-pattern replacement. Never call from process().
    std::array<std::uint8_t,kCells> snapshotCoherentForState() const {
        std::lock_guard<std::mutex> guard(writeMutex_);
        std::array<std::uint8_t,kCells> saved{};
        for(std::size_t i=0;i<kCells;++i)
            saved[i]=grid_[i].load(std::memory_order_relaxed);
        return saved;
    }
    bool select(std::size_t p) noexcept {
        if(p>=kPatterns)return false;
        selected_.store(static_cast<std::uint8_t>(p),std::memory_order_relaxed);
        return true;
    }
    std::uint8_t selected() const noexcept {
        return selected_.load(std::memory_order_relaxed);
    }
    static constexpr std::size_t index(std::size_t p,std::size_t r,
                                        std::size_t s) noexcept {
        return (p*kRows+r)*kSteps+s;
    }
    // Caller may read cells from any thread without a data race. A snapshot
    // is not a multi-edit transaction, only individually coherent step values.
    PatternBank::State snapshot() const noexcept {
        PatternBank copy;
        for(std::size_t p=0;p<kPatterns;++p)
            for(std::size_t r=0;r<kRows;++r)
                for(std::size_t s=0;s<kSteps;++s)
                    copy.set(p,r,s,get(p,r,s));
        copy.select(selected());
        return copy.save();
    }
    bool restore(const std::uint8_t* bytes,std::size_t size) noexcept {
        PatternBank data;
        if(!data.load(bytes,size))return false;
        std::array<std::uint8_t,kRows*kSteps> complete{};
        for(std::size_t p=0;p<kPatterns;++p) {
            for(std::size_t r=0;r<kRows;++r)
                for(std::size_t step=0;step<kSteps;++step)
                    complete[r*kSteps+step]=data.get(p,r,step);
            if(!replacePattern(p,complete.data(),complete.size()))return false;
        }
        select(data.selected());
        return true;
    }
private:
    std::array<std::atomic<std::uint8_t>,kCells> grid_{};
    mutable std::mutex writeMutex_{};
    std::array<std::atomic<std::uint8_t>,kPatterns> lengths_{};
    std::array<std::atomic<std::uint64_t>,kPatterns> version_{};
    std::atomic<std::uint8_t> selected_{0};
};
}
