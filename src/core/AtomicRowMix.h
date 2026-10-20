// 125A Drumator: packed, lock-free gain/pan transport to audio.
// Host UI changes both fields with a single RowMix message; the audio
// callback must never use gain from one edit and pan from another.
#pragma once
#include <atomic>
#include <cstdint>
namespace drumator {
class AtomicRowMix final {
public:
 static constexpr std::uint32_t encode(std::uint8_t gain,std::uint8_t pan) noexcept {
   return static_cast<std::uint32_t>(gain)|(static_cast<std::uint32_t>(pan)<<8);
 }
 static constexpr std::uint8_t gainPercent(std::uint32_t v) noexcept {
   return static_cast<std::uint8_t>(v&255u);
 }
 static constexpr std::uint8_t panEncoded(std::uint32_t v) noexcept {
   return static_cast<std::uint8_t>((v>>8)&255u);
 }
 static constexpr float gain(std::uint32_t v) noexcept {
   return static_cast<float>(gainPercent(v))/100.f;
 }
 static constexpr float pan(std::uint32_t v) noexcept {
   return static_cast<float>(static_cast<int>(panEncoded(v))-100)/100.f;
 }
 void store(std::uint8_t gain,std::uint8_t pan) noexcept {
   state_.store(encode(gain,pan),std::memory_order_release);
 }
 std::uint32_t snapshot() const noexcept {
   return state_.load(std::memory_order_acquire);
 }
private:
 std::atomic<std::uint32_t> state_{encode(100,100)};
};
static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "Row mix must remain lock-free for realtime audio");
}
