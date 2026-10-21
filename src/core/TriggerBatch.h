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
        events_[size_]=trigger;
        events_[size_].arrival=static_cast<std::uint32_t>(size_);
        ++size_;
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
        events_[size_]={static_cast<std::uint32_t>(offset),
                         static_cast<std::uint8_t>(pitch-36),0,
                         static_cast<std::uint8_t>(mapped),0,
                         static_cast<std::uint32_t>(size_)};
        ++size_;
        return true;
    }
    void applyMuteSolo(std::uint32_t muted,std::uint32_t solo) noexcept {
        std::size_t kept=0;
        for(std::size_t i=0;i<size_;++i) {
            const auto bit=std::uint32_t{1}<<events_[i].row;
            if((muted&bit)!=0||(solo!=0&&(solo&bit)==0))continue;
            events_[kept++]=events_[i];
        }
        size_=kept;
    }
    // Absolute PPQ position plus pattern/row/step yield identical velocity
    // regardless of the number of host audio callbacks.
    void varyVelocity(const std::array<std::uint8_t,kRows>& amount,
                      double startPpq,double bpm,double sampleRate) noexcept {
        for(std::size_t i=0;i<size_;++i) {
            auto& hit=events_[i];
            const auto depth=amount[hit.row];
            if(depth==0)continue;
            const double musicalStep=(startPpq+
                static_cast<double>(hit.sampleOffset)*bpm/(60.0*sampleRate))*4.0;
            const auto stepIndex=std::isfinite(musicalStep)&&
                musicalStep>-1.0e12&&musicalStep<1.0e12?
                static_cast<std::int64_t>(std::floor(musicalStep+1.e-7)):0;
            std::uint32_t hash=static_cast<std::uint32_t>(stepIndex)*
                0x9e3779b9u^static_cast<std::uint32_t>(hit.pattern+1)*0x85ebca6bu^
                static_cast<std::uint32_t>(hit.row+1)*0xc2b2ae35u^
                static_cast<std::uint32_t>(hit.step+1)*0x27d4eb2fu;
            hash^=hash>>16;hash*=0x7feb352du;hash^=hash>>15;
            const int deviation=static_cast<int>(hash%201u)-100;
            const int offset=deviation*static_cast<int>(depth)/100;
            hit.velocity=static_cast<std::uint8_t>(
                std::clamp(static_cast<int>(hit.velocity)+offset,1,127));
        }
    }
    void sort() noexcept {
        std::sort(events_.begin(),events_.begin()+size_,
            [](const Trigger& a,const Trigger& b){
                if(a.sampleOffset!=b.sampleOffset)return a.sampleOffset<b.sampleOffset;
                if(a.row!=b.row)return a.row<b.row;
                // Keep simultaneous same-row hits in their merge order.
                // An unrelated later event or block split must not change
                // which accepted choke voice survives. No stable_sort alloc.
                return a.arrival<b.arrival;
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
