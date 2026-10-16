// M15: deterministic mutually consistent mute/solo gating.
// Pure, bounded, allocation-free logic suitable for use at audio block boundaries.
#pragma once
#include "SequencerCore.h"
#include <array>
#include <cstddef>
namespace drumator {
struct RowMuteSolo {
 std::array<bool,kRows> mute{};
 std::array<bool,kRows> solo{};
 bool audible(std::size_t row) const noexcept {
   if(row>=kRows||mute[row])return false;
   bool anySolo=false;
   for(bool selected:solo)anySolo|=selected;
   return !anySolo||solo[row];
 }
};
} // namespace drumator
