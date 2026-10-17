#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace drumator {
// Control-thread only. Decode FLAC to portable 32-bit float WAV for the existing
// tested sample/state pipeline. On failure, out remains unchanged.
bool flacToWav(const std::uint8_t* data,std::size_t size,
               std::vector<std::uint8_t>& out) noexcept;
}
