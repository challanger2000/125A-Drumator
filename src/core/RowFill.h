// Deterministic 32-step row fills (control thread).
#pragma once
#include "SequencerCore.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace drumator {
enum class RowFill : std::uint8_t { Every2, Every4, Every8, Snare24, All, Clear, Offbeat8, Gallop, SnareGhost };
using RowVelocities = std::array<std::uint8_t,kSteps>;
// Replacement semantics. Every-N fills start at visible step 1.
constexpr RowVelocities filledRow(RowFill mode,std::uint8_t velocity=127) noexcept {
 RowVelocities result{};
 if(mode==RowFill::Clear)return result;
 if(mode==RowFill::Offbeat8){for(std::size_t s=1;s<kSteps;s+=2)result[s]=velocity;return result;}
 if(mode==RowFill::Gallop){for(std::size_t s=0;s<kSteps;s+=4){result[s]=velocity;result[s+2]=static_cast<std::uint8_t>(velocity*3u/4u);result[s+3]=static_cast<std::uint8_t>(velocity*2u/3u);}return result;}
 if(mode==RowFill::SnareGhost){for(std::size_t s=0;s<kSteps;s+=16){result[s+4]=velocity;result[s+12]=velocity;result[s+11]=static_cast<std::uint8_t>(velocity/3u);result[s+15]=static_cast<std::uint8_t>(velocity/3u);}return result;}
 if(mode==RowFill::Snare24) {
   for(std::size_t s=4;s<kSteps;s+=8)result[s]=velocity;
   return result;
 }
 const std::size_t interval=mode==RowFill::Every2?2:
   mode==RowFill::Every4?4:mode==RowFill::Every8?8:1;
 for(std::size_t s=0;s<kSteps;++s)if(s%interval==0)result[s]=velocity;
 return result;
}
}
