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
 template<class Callback>
 static ScheduleResult run(const PatternBank& b,const Clock& c,Callback&& emit) noexcept {
   if(!std::isfinite(c.ppq)||!std::isfinite(c.bpm)||!std::isfinite(c.rate)||
      c.bpm<=0||c.rate<=0||c.frames<0)return {ScheduleStatus::InvalidClock,0};
   if(!c.playing||!c.frames)return {};
   const long double start=static_cast<long double>(c.ppq)*4.0L;
   const long double stepSamples=15.0L*static_cast<long double>(c.rate)/
                                 static_cast<long double>(c.bpm);
   if(!std::isfinite(stepSamples)||stepSamples<=0)return {ScheduleStatus::InvalidClock,0};
   const long double end=start+static_cast<long double>(c.frames)/stepSamples;
   const auto tol=8.0L*std::numeric_limits<double>::epsilon()*
       std::max(1.0L,std::max(std::fabs(start),std::fabs(end)));
   const auto beginFloat=std::ceil(start-tol),endFloat=std::ceil(end-tol);
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
     const auto position=(static_cast<long double>(absolute)-start)*stepSamples;
     const auto eps=ppqErrorInSamples+
       16.0L*std::numeric_limits<double>::epsilon()*
       std::max(1.0L,std::fabs(position));
     const auto frame=std::ceil(position-eps);
     if(frame<0||frame>=c.frames)continue;
     const auto index=static_cast<std::size_t>(
       (absolute%static_cast<std::int64_t>(kSteps)+
        static_cast<std::int64_t>(kSteps))%static_cast<std::int64_t>(kSteps));
     const auto p=b.selected();
     for(std::size_t row=0;row<kRows;++row){
       const auto velocity=b.get(p,row,index);
       if(velocity){
         emit(Trigger{static_cast<std::uint32_t>(frame),
                      static_cast<std::uint8_t>(row),
                      static_cast<std::uint8_t>(index),velocity,p});
         ++result.count;
       }
     }
   }
   return result;
 }
};
} // namespace drumator
