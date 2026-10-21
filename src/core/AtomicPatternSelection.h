#pragma once
#include <atomic>
#include <cstdint>
namespace drumator {
// Pattern selection plus host-event generation in one atomic modification order.
// A kit restore cannot overwrite a newer host automation event.
class AtomicPatternSelection final {
 std::atomic<std::uint64_t> state_{0};
public:
 std::uint32_t load(std::memory_order order=std::memory_order_seq_cst) const noexcept {
   return static_cast<std::uint32_t>(state_.load(order)&7u);
 }
 void store(std::uint32_t value,std::memory_order order=std::memory_order_seq_cst) noexcept {
   auto old=state_.load(std::memory_order_relaxed);
   const auto desiredMask=static_cast<std::uint64_t>(value&7u);
   while(!state_.compare_exchange_weak(old,(old&~std::uint64_t{7})|desiredMask,
           order,std::memory_order_relaxed)) {}
 }
 std::uint64_t snapshot() const noexcept {
   return state_.load(std::memory_order_acquire);
 }
 // Actual incoming MIDI pattern changes are no less authoritative than
 // host automation while an old kit is being recalled. They must advance
 // the same event generation (even for same-value / A->B->A sequences).
 // Plain store() remains for initialization and non-event setup.
 void midiSubmit(std::uint32_t value) noexcept { hostSubmit(value); }
 void hostSubmit(std::uint32_t value) noexcept {
   auto old=state_.load(std::memory_order_relaxed);
   std::uint64_t next;
   do {next=((old>>3)+1u)*8u|(value&7u);}
   while(!state_.compare_exchange_weak(old,next,std::memory_order_acq_rel,
                                       std::memory_order_relaxed));
 }
 void restoreIfUnchanged(std::uint64_t previous,std::uint32_t kitValue) noexcept {
   const auto desired=(previous&~std::uint64_t{7})|(kitValue&7u);
   state_.compare_exchange_strong(previous,desired,std::memory_order_acq_rel,
                                  std::memory_order_relaxed);
 }
};
static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "Pattern arbitration must remain lock-free on the audio thread");
}
