#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <limits>
namespace drumator {
constexpr std::size_t kPatterns=8,kRows=16,kSteps=32,kCells=kPatterns*kRows*kSteps;
constexpr std::size_t kStateSize=6+kCells;
class PatternBank {
public:
 using State=std::array<std::uint8_t,kStateSize>;
 bool set(std::size_t p,std::size_t r,std::size_t s,unsigned v) noexcept {
   if(p>=kPatterns||r>=kRows||s>=kSteps||v>127) return false;
   grid_[p][r][s]=static_cast<std::uint8_t>(v);return true;
 }
 std::uint8_t get(std::size_t p,std::size_t r,std::size_t s) const noexcept {
   return p<kPatterns&&r<kRows&&s<kSteps?grid_[p][r][s]:0;
 }
 bool select(std::size_t p) noexcept {
   if(p>=kPatterns)return false;selected_=static_cast<std::uint8_t>(p);return true;
 }
 std::uint8_t selected() const noexcept{return selected_;}
 State save() const noexcept {
   State b{};b[0]='1';b[1]='2';b[2]='5';b[3]='D';b[4]=1;b[5]=selected_;
   std::size_t n=6;
   for(const auto& p:grid_)for(const auto& r:p)for(auto v:r)b[n++]=v;
   return b;
 }
 // Malformed state must NEVER partially overwrite existing pattern data.
 // This snapshot is control-thread/single-owner; concurrent editing is forbidden.
 bool load(const std::uint8_t* b,std::size_t n) noexcept {
   if(!b||n!=kStateSize||b[0]!='1'||b[1]!='2'||b[2]!='5'||
      b[3]!='D'||b[4]!=1||b[5]>=kPatterns)return false;
   PatternBank candidate;candidate.selected_=b[5];std::size_t at=6;
   for(auto& p:candidate.grid_)for(auto& r:p)for(auto& v:r){
     if(b[at]>127)return false;v=b[at++];
   }
   *this=candidate;return true;
 }
private:
 std::array<std::array<std::array<std::uint8_t,kSteps>,kRows>,kPatterns> grid_{};
 std::uint8_t selected_{0};
};
struct Clock {
 double ppq{0},bpm{120},rate{44100};
 std::int32_t frames{0};
 bool playing{false};
 double swing{0.0}; // 0..0.5 of a 16th step, delayed odd steps
 double humanize{0.0}; // 0..1 deterministic timing and velocity
 std::int64_t phaseOriginStep{0}; // musical cycle reset at host transport start
};
struct Trigger {
 std::uint32_t sampleOffset;
 std::uint8_t row,step,velocity,pattern;
};
enum class ScheduleStatus {Ok,InvalidClock,TooManyBoundaries};
struct ScheduleResult {ScheduleStatus status{ScheduleStatus::Ok};std::uint32_t count{0};};
class StepScheduler {
public:
 // Hard cap on work for pathological host clocks; no partial emission.
 static constexpr std::uint32_t maxBoundariesPerBlock=128;
 // 1/16-note steps at host absolute PPQ; block half-open [0, frames).
 // Calling thread owns an immutable PatternBank for entire call.
 template<class Bank,class Callback>
 static ScheduleResult run(const Bank& b,const Clock& c,Callback&& emit) noexcept {
   if(!std::isfinite(c.ppq)||!std::isfinite(c.bpm)||!std::isfinite(c.rate)||
      c.bpm<=0||c.rate<=0||c.frames<0||
      !std::isfinite(c.swing)||c.swing<0.0||c.swing>0.5||
      !std::isfinite(c.humanize)||c.humanize<0.0||c.humanize>1.0)return {ScheduleStatus::InvalidClock,0};
   if(!c.playing||!c.frames)return {};
   const long double start=static_cast<long double>(c.ppq)*4.0L;
   const long double stepSamples=15.0L*static_cast<long double>(c.rate)/
                                 static_cast<long double>(c.bpm);
   if(!std::isfinite(stepSamples)||stepSamples<=0)return {ScheduleStatus::InvalidClock,0};
   const long double end=start+static_cast<long double>(c.frames)/stepSamples;
   const auto tol=8.0L*std::numeric_limits<double>::epsilon()*
       std::max(1.0L,std::max(std::fabs(start),std::fabs(end)));
   const auto beginFloat=std::ceil(start-1.0L-tol),endFloat=std::ceil(end-tol);
   const auto minIndex=static_cast<long double>(std::numeric_limits<std::int64_t>::min()+1);
   const auto maxIndex=static_cast<long double>(std::numeric_limits<std::int64_t>::max()-1);
   if(!std::isfinite(beginFloat)||!std::isfinite(endFloat)||
      beginFloat<minIndex||endFloat>maxIndex)return {ScheduleStatus::InvalidClock,0};
   if(endFloat<=beginFloat)return {};
   if(endFloat-beginFloat>maxBoundariesPerBlock)
       return {ScheduleStatus::TooManyBoundaries,0};
   // IEEE754 double host-PPQ quantization propagates through the
   // multiplication into sample positions. Bound that error explicitly.
   const auto ppqErrorInSamples=16.0L*std::numeric_limits<double>::epsilon()*
       std::max(1.0L,std::max(std::fabs(start),std::fabs(end)))*stepSamples;
   // Unresolvable sub-sample time coordinates must fail before callbacks.
   if(ppqErrorInSamples>=0.5L)return {ScheduleStatus::InvalidClock,0};
   ScheduleResult result{};
   const auto first=static_cast<std::int64_t>(beginFloat);
   const auto last=static_cast<std::int64_t>(endFloat);
   for(auto absolute=first;absolute<last;++absolute){
     const auto swingOffset=(absolute%2!=0)?static_cast<long double>(c.swing):0.0L;
     const auto seed=static_cast<std::uint64_t>(absolute);
     const auto mixed=(seed^(seed>>30))*0xbf58476d1ce4e5b9ULL;
     const auto mixed2=(mixed^(mixed>>27))*0x94d049bb133111ebULL;
     const auto bits=mixed2^(mixed2>>31);
     // Delay only: bounded to 10% of one step, never reorders steps.
     const auto jitter=static_cast<long double>(c.humanize)*
                       static_cast<long double>(bits&0xffffu)/65535.0L*0.1L;
     const auto position=(static_cast<long double>(absolute)+swingOffset+jitter-start)*stepSamples;
     const auto eps=ppqErrorInSamples+
       16.0L*std::numeric_limits<double>::epsilon()*
       std::max(1.0L,std::fabs(position));
     const auto frame=std::ceil(position-eps);
     if(frame<0||frame>=c.frames)continue;
     const auto index=static_cast<std::size_t>(
       ((absolute-c.phaseOriginStep)%static_cast<std::int64_t>(kSteps)+
        static_cast<std::int64_t>(kSteps))%static_cast<std::int64_t>(kSteps));
     auto p=b.selected();
     auto patternStep=index;
     // C++23: song-aware banks may supply a musical position mapping.
     // Legacy PatternBank retains its existing 32-step loop unchanged.
     if constexpr(requires { b.mapStep(absolute,c.phaseOriginStep); }) {
       const auto songPosition=b.mapStep(absolute,c.phaseOriginStep);
       p=songPosition.pattern;
       patternStep=songPosition.step;
     }
     for(std::size_t row=0;row<kRows;++row){
       const auto velocity=b.get(p,row,patternStep);
       if(velocity){
         const auto rowSeed=bits^(static_cast<std::uint64_t>(row)+1)*0x9e3779b97f4a7c15ULL;
         const int deviation=static_cast<int>((rowSeed>>24)%25)-12;
         const int varied=std::clamp(static_cast<int>(velocity)+
                       static_cast<int>(std::lround(deviation*c.humanize)),1,127);
         emit(Trigger{static_cast<std::uint32_t>(frame),
                      static_cast<std::uint8_t>(row),
                      static_cast<std::uint8_t>(index),
                      static_cast<std::uint8_t>(varied),p});
         ++result.count;
       }
     }
   }
   return result;
 }
};
} // namespace drumator
