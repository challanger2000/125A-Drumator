// Deterministic 32-step row fills (control thread).
#pragma once
#include "SequencerCore.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace drumator {
enum class RowFill : std::uint8_t { Every2, Every4, Every8, All, Clear };
using RowVelocities = std::array<std::uint8_t,kSteps>;
// Replacement semantics. Every-N fills start at visible step 1.
constexpr RowVelocities filledRow(RowFill mode,std::uint8_t velocity=127) noexcept {
 RowVelocities result{};
 if(mode==RowFill::Clear)return result;
 const std::size_t interval=mode==RowFill::Every2?2:
   mode==RowFill::Every4?4:mode==RowFill::Every8?8:1;
 for(std::size_t s=0;s<kSteps;++s)if(s%interval==0)result[s]=velocity;
 return result;
}
}
