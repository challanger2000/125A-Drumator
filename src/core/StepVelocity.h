// UI-only velocity cycle. Existing stored velocities (0..127) remain valid.
// Shift-click advances to the next stronger level, wrapping strong -> off.
#pragma once
#include <cstdint>
namespace drumator {
constexpr std::uint8_t nextStepVelocity(std::uint8_t value) noexcept {
 return value<32?32:value<64?64:value<96?96:value<127?127:0;
}
}
