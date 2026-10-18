// A single atomic gate linearizes entry of audio blocks with a control-side
// kit restore. Audio takes only bounded atomic operations; only the control
// thread may wait for existing audio readers to complete.
#pragma once
#include <atomic>
#include <cstdint>
#include <thread>
namespace drumator {
static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "Kit restore audio gate requires lock-free uint64 atomics");
class KitRestoreGate final {
 static constexpr std::uint64_t kBusy=std::uint64_t{1}<<63;
 std::atomic<std::uint64_t> state_{0};
public:
 bool audioEnter() noexcept {
   return (state_.fetch_add(1,std::memory_order_acq_rel)&kBusy)==0;
 }
 void audioLeave() noexcept {state_.fetch_sub(1,std::memory_order_release);}
 void controlBegin() noexcept {
   state_.fetch_or(kBusy,std::memory_order_acq_rel);
   while((state_.load(std::memory_order_acquire)&~kBusy)!=0)
     std::this_thread::yield();
 }
 void controlEnd() noexcept {
   state_.fetch_and(~kBusy,std::memory_order_release);
 }
};
class AudioGateEntry final {
 KitRestoreGate& gate_;
 bool allowed_;
public:
 explicit AudioGateEntry(KitRestoreGate& gate) noexcept
   :gate_(gate),allowed_(gate.audioEnter()) {}
 ~AudioGateEntry(){gate_.audioLeave();}
 bool allowed() const noexcept {return allowed_;}
};
class ControlRestoreEntry final {
 KitRestoreGate& gate_;
public:
 explicit ControlRestoreEntry(KitRestoreGate& gate) noexcept:gate_(gate) {
   gate_.controlBegin();
 }
 ~ControlRestoreEntry(){gate_.controlEnd();}
};
}
