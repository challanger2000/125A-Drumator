// 125A Drumator - bounded, allocation-free MIDI row trigger accumulator.
// This does not touch the host SDK; the VST3 adapter validates event types.
#pragma once
#include "SequencerCore.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace drumator {

class TriggerBatch final {
public:
    static constexpr std::size_t kCapacity=kRows*StepScheduler::maxBoundariesPerBlock;
    bool appendStep(const Trigger& trigger) noexcept {
        if(size_>=kCapacity||trigger.row>=kRows||trigger.step>=kSteps||
           trigger.pattern>=kPatterns||trigger.velocity==0||trigger.velocity>127)
            return false;
        events_[size_++]=trigger;
        return true;
    }
    // GM-inspired contiguous trigger range: note 36 -> row 1, 51 -> row 16.
    // Note-off is ignored because Drumator plays one-shots.
    bool appendNoteOn(std::int32_t pitch,float velocity,std::int32_t offset,
                      std::uint32_t frames) noexcept {
        if(pitch<36||pitch>=36+static_cast<std::int32_t>(kRows)||
           offset<0||static_cast<std::uint32_t>(offset)>=frames||
           !std::isfinite(velocity)||velocity<=0.f||velocity>1.f||
           size_>=kCapacity)return false;
        const auto mapped=std::clamp(static_cast<int>(std::lround(
            static_cast<double>(velocity)*127.0)),1,127);
        events_[size_++]={static_cast<std::uint32_t>(offset),
                         static_cast<std::uint8_t>(pitch-36),0,
                         static_cast<std::uint8_t>(mapped),0};
        return true;
    }
    void sort() noexcept {
        std::sort(events_.begin(),events_.begin()+size_,
            [](const Trigger& a,const Trigger& b){
                if(a.sampleOffset!=b.sampleOffset)return a.sampleOffset<b.sampleOffset;
                return a.row<b.row;
            });
    }
    const Trigger* data() const noexcept{return events_.data();}
    std::size_t size() const noexcept{return size_;}
    void clear() noexcept{size_=0;}
private:
    std::array<Trigger,kCapacity> events_{};
    std::size_t size_{0};
};
}
