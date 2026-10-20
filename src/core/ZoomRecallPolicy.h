#pragma once
#include <cstddef>
namespace drumator {
// Host-controlled frames can reject both zoom and rollback. Permit at most
// one frame-changing request per state restore; never half-update many views.
constexpr bool zoomRecallIsAtomic(std::size_t requiredViewChanges) noexcept {
 return requiredViewChanges<=1;
}
}
