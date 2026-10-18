// 125A Drumator - independent sample-voice rendering core.
// Design: prepared, immutable planar PCM and bandlimited interpolation.
// This module is NOT the VST3 adapter or a file decoder.
#pragma once
#include "SequencerCore.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
#include <new>

namespace drumator {

constexpr std::size_t kVoiceLimit=64;
constexpr std::size_t kSincTaps=256; // maximum; actual CPU taps adapt to ratio
constexpr std::size_t kSincPhases=1024;

// Non-owning: backing PCM and the interpolation kernel must remain alive for
// EVERY voice referencing them, even after a row's sample is replaced.
// Loading, decoding and reclamation must be implemented off the audio thread.
struct SampleView {
    const float* left{nullptr};
    const float* right{nullptr}; // nullptr means mono; render left to both sides.
    std::size_t frames{0};
};

// Prepared on the CONTROL thread, never constructed/updated in process().
// Ratio = source frames advanced per output frame, includes source/output
// sample-rate ratio and semitone pitch multiplier 2^(semitones/12).
class SincKernel final {
public:
    bool prepare(double playbackRatio) noexcept {
        if (!std::isfinite(playbackRatio) || playbackRatio<=0.0) return false;
        constexpr double pi=3.141592653589793238462643383279502884;
        const double cutoff=std::min(1.0,1.0/playbackRatio);
        taps_=playbackRatio>=8.0?256:(playbackRatio>=4.0?128:(playbackRatio>=2.0?64:32));
        // Allocate on the control thread, not on the stack or audio thread.
        try { coefficients_.resize(kSincPhases*kSincTaps); }
        catch(const std::bad_alloc&) { ready_=false;return false; }
        for(std::size_t phase=0;phase<kSincPhases;++phase) {
            const double fraction=static_cast<double>(phase)/kSincPhases;
            double sum=0.0;
            for(std::size_t i=0;i<taps_;++i) {
                const double x=static_cast<double>(i)-static_cast<double>(taps_/2-1)-fraction;
                const double z=pi*cutoff*x;
                const double sinc=std::abs(z)<1e-13?1.0:std::sin(z)/z;
                const double t=2.0*pi*static_cast<double>(i)/(taps_-1);
                const double window=0.42-0.5*std::cos(t)+0.08*std::cos(2.0*t);
                const double weight=cutoff*sinc*window;
                coefficients_[phase*kSincTaps+i]=static_cast<float>(weight);
                sum+=weight;
            }
            if(!std::isfinite(sum) || std::abs(sum)<1e-12) return false;
            for(std::size_t i=0;i<taps_;++i)
                coefficients_[phase*kSincTaps+i]=static_cast<float>(
                    coefficients_[phase*kSincTaps+i]/sum);
        }
        ratio_=playbackRatio;
        ready_=true;
        return true;
    }
    double ratio() const noexcept {return ratio_;}
    std::size_t taps() const noexcept {return taps_;}
    bool ready() const noexcept {return ready_;}
    const float* phase(std::size_t index) const noexcept {
        return coefficients_.data()+index*kSincTaps;
    }
private:
    std::vector<float> coefficients_{};
    double ratio_{0.0};
    std::size_t taps_{32};
    bool ready_{false};
};

struct RowSound {
    SampleView sample{};
    const SincKernel* kernel{nullptr};
    float gain{1.f}; // linear amplitude, 1 = 0 dB.
    float pan{0.f};  // -1 left, 0 center, +1 right.
    std::uint8_t chokeGroup{0}; // 0 = disabled.
    std::uint8_t outputBus{0}; // assigned when launching a voice
    bool mute{false};
    const void* ownerToken{nullptr}; // audio-owned identity for safe deferred reclaim
};

enum class RenderStatus { Ok,InvalidInput,InvalidEvents };
struct RenderResult {
    RenderStatus status{RenderStatus::Ok};
    std::uint32_t accepted{0};
    std::uint32_t rejectedCapacity{0};
    std::uint32_t activeVoices{0};
};

// Single audio-thread owner. setRow is only safe at the same synchronization
// boundary as process, never concurrently with another thread. No callbacks
// into the host. Processing accepts validated, sorted per-block trigger events.
class SampleVoiceEngine final {
public:
    bool setOutputRate(double rate, double chokeFadeMs=2.0) noexcept {
        if(!std::isfinite(rate)||rate<=0||
           !std::isfinite(chokeFadeMs)||chokeFadeMs<=0||
           chokeFadeMs>1000.0)return false;
        const double count=std::ceil(rate*chokeFadeMs/1000.0);
        if(count>static_cast<double>(std::numeric_limits<std::uint32_t>::max()))
            return false;
        fadeFrames_=std::max(std::uint32_t{1},static_cast<std::uint32_t>(count));
        return true;
    }
    bool setRow(std::size_t row, const RowSound& sound) noexcept {
        if(row>=kRows||!std::isfinite(sound.gain)||sound.gain<0.f||
           !std::isfinite(sound.pan)||sound.pan< -1.f||sound.pan>1.f)
            return false;
        if(sound.sample.frames!=0) {
            if(!sound.sample.left||!sound.kernel||!sound.kernel->ready()||
                sound.sample.frames>static_cast<std::size_t>(
                    std::numeric_limits<std::int64_t>::max()-64))
                return false;
        }
        rows_[row]=sound;
        rows_[row].outputBus=static_cast<std::uint8_t>(row); // legacy output mapping
        return true;
    }
    // Called only by the audio-thread owner at block boundaries.
    // Existing one-shot voices retain their launch gain/pan.
    bool setRowMix(std::size_t row,float gain,float pan) noexcept {
        if(row>=kRows||!std::isfinite(gain)||gain<0.f||gain>2.f||
           !std::isfinite(pan)||pan< -1.f||pan>1.f)return false;
        rows_[row].gain=gain;
        rows_[row].pan=pan;
        return true;
    }
    bool setRowOutput(std::size_t row,std::uint8_t bus) noexcept {
        if(row>=kRows||bus>=kRows)return false;
        rows_[row].outputBus=bus;return true;
    }
    bool setRowChoke(std::size_t row,std::uint8_t group) noexcept {
        if(row>=kRows||group>4)return false;
        rows_[row].chokeGroup=group;
        return true;
    }
    // reset() only at a host lifecycle discontinuity; not for ordinary
    // sample replacement (existing one-shots continue).
    void reset() noexcept {
        for(auto& v:voices_)v.active=false;
    }
    // Audio-thread-only: are there voices still reading this immutable asset?
    bool references(const void* token) const noexcept {
        if(!token)return false;
        for(const auto& v:voices_)
            if(v.active && v.ownerToken==token)return true;
        return false;
    }
    std::uint32_t active() const noexcept {
        std::uint32_t count=0;
        for(const auto& v:voices_)if(v.active)++count;
        return count;
    }

    RenderResult render(const Trigger* triggers,std::size_t triggerCount,
                        float* left,float* right,std::uint32_t frames,
                        float* const* stemLeft=nullptr,
                        float* const* stemRight=nullptr) noexcept {
        if((left==nullptr)!=(right==nullptr)||(triggerCount&&!triggers))return {RenderStatus::InvalidInput};
        // Reject malformed event arrays atomically, before modifying voice
        // state or buffers. No event reordering/allocations in audio callback.
        if(triggerCount>static_cast<std::size_t>(kRows)*
                        StepScheduler::maxBoundariesPerBlock)
            return {RenderStatus::InvalidEvents};
        for(std::size_t n=0;n<triggerCount;++n) {
            const auto& e=triggers[n];
            if(e.row>=kRows||e.step>=kSteps||e.pattern>=kPatterns||
               e.velocity==0||e.velocity>127||e.sampleOffset>=frames||
               (n>0&&e.sampleOffset<triggers[n-1].sampleOffset))
                return {RenderStatus::InvalidEvents};
        }
        if(left) {
            std::fill_n(left,frames,0.f);
            std::fill_n(right,frames,0.f);
        }
        if(stemLeft&&stemRight)for(std::size_t bus=1;bus<kRows;++bus) {
           if(stemLeft[bus]&&stemRight[bus]) {
             std::fill_n(stemLeft[bus],frames,0.f);
             std::fill_n(stemRight[bus],frames,0.f);
           }
        }
        RenderResult result{};
        std::size_t event=0;
        for(std::uint32_t f=0;f<frames;++f) {
            while(event<triggerCount&&triggers[event].sampleOffset==f) {
                fire(triggers[event],result);
                ++event;
            }
            double l=0.0,r=0.0;
            for(auto& voice:voices_) {
                if(!voice.active)continue;
                if(voice.position>=static_cast<double>(voice.sample.frames)) {
                    voice.active=false;
                    continue;
                }
                const double x=voice.position;
                const auto base=static_cast<std::int64_t>(x);
                const double fractional=x-static_cast<double>(base);
                const auto phase=std::min<std::size_t>(
                    static_cast<std::size_t>(fractional*kSincPhases),kSincPhases-1);
                const auto* coeff=voice.kernel->phase(phase);
                double vl=0.0,vr=0.0;
                for(std::size_t i=0;i<voice.kernel->taps();++i) {
                    const auto index=base+static_cast<std::int64_t>(i)-
                        static_cast<std::int64_t>(voice.kernel->taps()/2-1);
                    if(index<0||static_cast<std::size_t>(index)>=voice.sample.frames)
                        continue; // documented zero-padding at sample boundaries
                    const auto w=static_cast<double>(coeff[i]);
                    const auto at=static_cast<std::size_t>(index);
                    vl+=static_cast<double>(voice.sample.left[at])*w;
                    vr+=static_cast<double>(voice.sample.right?
                                             voice.sample.right[at]:
                                             voice.sample.left[at])*w;
                }
                double envelope=1.0;
                if(voice.releaseLeft>0) {
                    envelope=static_cast<double>(voice.releaseLeft)/
                             static_cast<double>(voice.releaseTotal);
                    --voice.releaseLeft;
                }
                const auto a=static_cast<float>(vl*voice.gainL*envelope);
                const auto b=static_cast<float>(vr*voice.gainR*envelope);
                if(stemLeft&&stemRight&&voice.outputBus>0&&
                   stemLeft[voice.outputBus]&&stemRight[voice.outputBus]) {
                   stemLeft[voice.outputBus][f]+=a;
                   stemRight[voice.outputBus][f]+=b;
                }else {
                   l+=a;r+=b;
                }
                voice.position+=voice.kernel->ratio();
                if((voice.releaseTotal>0&&voice.releaseLeft==0)||
                   voice.position>=static_cast<double>(voice.sample.frames))
                    voice.active=false;
            }
            if(left) {
                left[f]=static_cast<float>(l);
                right[f]=static_cast<float>(r);
            }
        }
        result.activeVoices=active();
        return result;
    }
private:
    struct Voice {
        SampleView sample{};
        const SincKernel* kernel{nullptr};
        double position{0.0};
        double gainL{0.0},gainR{0.0};
        std::uint32_t releaseLeft{0},releaseTotal{0};
        std::uint8_t group{0};
        std::uint8_t row{0};
        std::uint8_t outputBus{0};
        const void* ownerToken{nullptr};
        bool active{false};
    };
    void fire(const Trigger& event,RenderResult& result) noexcept {
        const auto& row=rows_[event.row];
        if(row.mute||!row.sample.left||!row.sample.frames||
           !row.kernel||!row.kernel->ready())return;
        // Select capacity before applying choke: a rejected hit must not
        // alter any voice already sounding.
        Voice* freeVoice=nullptr;
        for(auto& voice:voices_)if(!voice.active) {freeVoice=&voice;break;}
        if(!freeVoice){++result.rejectedCapacity;return;}
        if(row.chokeGroup)for(auto& voice:voices_) {
            if(voice.active&&voice.group==row.chokeGroup&&voice.releaseTotal==0) {
                voice.releaseTotal=fadeFrames_;
                voice.releaseLeft=fadeFrames_;
            }
        }
        {
            auto& v=*freeVoice;
            constexpr double pi=3.141592653589793238462643383279502884;
            const double pan=static_cast<double>(row.pan);
            const double velocity=static_cast<double>(event.velocity)/127.0;
            v.sample=row.sample;
            v.kernel=row.kernel;
            v.position=0;
            // Center pan=0: unity WAV sample amplitude on each channel.
            // This is a transparent stereo BALANCE law, not a +dB booster:
            // left falls smoothly to 0 when panned right, and vice versa.
            // No normalization, no limiter, no hidden gain makeup.
            v.gainL=static_cast<double>(row.gain)*velocity*
                    (pan<=0.0?1.0:std::cos(pan*pi/2.0));
            v.gainR=static_cast<double>(row.gain)*velocity*
                    (pan>=0.0?1.0:std::cos(-pan*pi/2.0));
            v.group=row.chokeGroup;
            v.row=event.row;
            v.outputBus=row.outputBus;
            v.ownerToken=row.ownerToken;
            v.releaseTotal=0;
            v.releaseLeft=0;
            v.active=true;
            ++result.accepted;
            return;
        }
    }
    std::array<RowSound,kRows> rows_{};
    std::array<Voice,kVoiceLimit> voices_{};
    std::uint32_t fadeFrames_{88}; // default only before setOutputRate()
};
} // namespace drumator
