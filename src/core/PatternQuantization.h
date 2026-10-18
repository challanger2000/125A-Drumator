#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace drumator {
inline std::uint32_t patternFromNormalized(double value) noexcept {
 if(!std::isfinite(value))return 0;
 return std::min<std::uint32_t>(7u,static_cast<std::uint32_t>(
      std::clamp(value,0.0,1.0)*8.0));
}
}
