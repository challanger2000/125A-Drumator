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
        for(std::size_t p=0;p<kPatterns;++p)
            for(std::size_t s=0;s<kSteps;s+=4)
                set(p,0,s,(s%8==0)?127:90);
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
                         std::array<std::uint8_t,kRows*kSteps>& out) const noexcept {
        if(p>=kPatterns)return false;
        const auto before=version_[p].load(std::memory_order_acquire);
        if(before&1u)return false;
        std::array<std::uint8_t,kRows*kSteps> candidate{};
        for(std::size_t i=0;i<candidate.size();++i)
            candidate[i]=grid_[p*kRows*kSteps+i].load(std::memory_order_relaxed);
        const auto after=version_[p].load(std::memory_order_acquire);
        if(before!=after|| (after&1u))return false;
        out=candidate;
        return true;
    }
    bool replacePattern(std::size_t p,const std::uint8_t* values,std::size_t count) noexcept {
        if(p>=kPatterns||!values||count!=kRows*kSteps)return false;
        for(std::size_t i=0;i<count;++i)if(values[i]>127)return false;
        std::lock_guard<std::mutex> guard(writeMutex_);
        version_[p].fetch_add(1,std::memory_order_acq_rel);
        for(std::size_t i=0;i<count;++i)
            grid_[p*kRows*kSteps+i].store(values[i],std::memory_order_relaxed);
        version_[p].fetch_add(1,std::memory_order_release);
        return true;
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
        for(std::size_t p=0;p<kPatterns;++p)
            for(std::size_t r=0;r<kRows;++r)
                for(std::size_t s=0;s<kSteps;++s)
                    set(p,r,s,data.get(p,r,s));
        select(data.selected());
        return true;
    }
private:
    std::array<std::atomic<std::uint8_t>,kCells> grid_{};
    mutable std::mutex writeMutex_{};
    std::array<std::atomic<std::uint64_t>,kPatterns> version_{};
    std::atomic<std::uint8_t> selected_{0};
};
}
