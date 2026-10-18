// Preserves host pattern automation arriving during the control-thread
// installation of a complete kit. Submit is bounded and lock-free.
#pragma once
#include <atomic>
#include <cstdint>
namespace drumator {
class PatternRestoreLatch final {
 std::atomic<std::uint32_t> pending_{0};
 std::atomic<std::uint64_t> revision_{0};
public:
 std::uint64_t revision() const noexcept {
   return revision_.load(std::memory_order_acquire);
 }
 void submit(std::uint32_t value,std::atomic<std::uint32_t>& selected) noexcept {
   pending_.store(value,std::memory_order_relaxed);
   revision_.fetch_add(1,std::memory_order_release);
   selected.store(value,std::memory_order_release);
 }
 void finish(std::uint64_t start,std::atomic<std::uint32_t>& selected) noexcept {
   if(revision_.load(std::memory_order_acquire)!=start)
     selected.store(pending_.load(std::memory_order_acquire),
                    std::memory_order_release);
 }
};
}
