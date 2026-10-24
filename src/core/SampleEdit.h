// 125A Drumator sample-editor model. No allocation or I/O on the audio thread.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace drumator {
struct SampleEdit {
 // Half-open [start,end); end == total frames is legal.
 std::uint32_t start{0},end{0};
 std::uint32_t fadeIn{0},fadeOut{0};
 float gain{1.f};
 static SampleEdit reset(std::uint32_t frames) noexcept {
   return {0,frames,0,0,1.f};
 }
 bool valid(std::uint32_t frames) const noexcept {
   if(!frames||start>=end||end>frames||!std::isfinite(gain)||gain<0.f||gain>4.f)
     return false;
   const auto span=end-start;
   return fadeIn<=span&&fadeOut<=span&&
          static_cast<std::uint64_t>(fadeIn)+fadeOut<=span;
 }
 std::uint32_t length() const noexcept {return end-start;}
 // Envelope uses absolute source-frame positions and is bounded and deterministic.
 float amplitude(std::uint32_t sourceFrame) const noexcept {
   if(sourceFrame<start||sourceFrame>=end)return 0.f;
   const auto local=sourceFrame-start;
   float a=gain;
   if(fadeIn>0&&local<fadeIn)
     a*=static_cast<float>(local)/static_cast<float>(fadeIn);
   if(fadeOut>0&&end-sourceFrame<=fadeOut)
     a*=static_cast<float>(end-1-sourceFrame)/static_cast<float>(fadeOut);
   return a;
 }
};
// Compute a visible waveform envelope on the control/UI thread. Each pixel
// covers a half-open source-frame interval; no sample is omitted at boundaries.
struct WaveformRange {float minimum{0.f},maximum{0.f};};
inline WaveformRange waveformRange(const float* samples,std::size_t frames,
                                  std::size_t begin,std::size_t end) noexcept {
 WaveformRange result{};
 if(!samples||!frames||begin>=end||end>frames)return result;
 result.minimum=result.maximum=samples[begin];
 for(std::size_t i=begin+1;i<end;++i){
   result.minimum=std::min(result.minimum,samples[i]);
   result.maximum=std::max(result.maximum,samples[i]);
 }
 return result;
}
inline std::uint32_t waveformFrameAt(double x,double width,
                                    std::uint32_t first,std::uint32_t last) noexcept {
 if(!(std::isfinite(x)&&std::isfinite(width)&&width>0&&last>=first))return first;
 const double fraction=std::clamp(x/width,0.,1.);
 return first+static_cast<std::uint32_t>(
   std::floor(static_cast<double>(last-first)*fraction));
}
} // namespace drumator
