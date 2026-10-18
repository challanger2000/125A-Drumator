#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace drumator {
inline std::uint32_t patternFromNormalized(double value) noexcept {
 if(!std::isfinite(value))return 0;
 // Preserve the original Drumator 0.1.0 automation thresholds.
 return static_cast<std::uint32_t>(
      std::lround(std::clamp(value,0.0,1.0)*7.0));
}
}
