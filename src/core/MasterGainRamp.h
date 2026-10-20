// 125A Drumator: allocation-free, sample-rate-aware master gain ramp.
// prepare/reset only while processing is quiescent; request/next only on audio.
#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
namespace drumator {
class MasterGainRamp final {
public:
 static constexpr double kTransitionSeconds=0.005; // 5 ms de-click interval
 bool prepare(double rate) noexcept {
   if(!std::isfinite(rate)||rate<=0.)return false;
   const double frames=std::ceil(rate*kTransitionSeconds);
   if(!std::isfinite(frames)||
      frames>static_cast<double>(std::numeric_limits<std::uint32_t>::max()))
     return false;
   duration_=frames<1.?1u:static_cast<std::uint32_t>(frames);
   return true;
 }
 void reset(float gain) noexcept {
   current_=target_=static_cast<double>(gain);
   increment_=0.;remaining_=0;
 }
 void request(float gain) noexcept {
   const double target=static_cast<double>(gain);
   if(target==target_)return;
   target_=target;
   remaining_=duration_;
   increment_=(target_-current_)/static_cast<double>(remaining_);
 }
 float next() noexcept {
   if(remaining_!=0) {
     --remaining_;
     if(remaining_==0)current_=target_;
     else current_+=increment_;
   }
   return static_cast<float>(current_);
 }
 bool settledAtUnity() const noexcept {
   return remaining_==0&&current_==1.0;
 }
private:
 double current_{1.0},target_{1.0},increment_{0.0};
 std::uint32_t duration_{1},remaining_{0};
};
} // namespace drumator
