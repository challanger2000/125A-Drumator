#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
namespace drumator {
// Prepare all allocating string copies before touching the live controller.
// After prepare(), commit() is nothrow: no failed recall can half-change a kit.
template<std::size_t Rows>
class PreparedKitNames final {
 public:
  using Names=std::array<std::string,Rows>;
  void prepare(const Names& desiredCustom,const Names& wavNames) {
    Names pendingCustom=desiredCustom;
    Names pendingLabels{};
    for(std::size_t i=0;i<Rows;++i)
      pendingLabels[i]=wavNames[i].empty()?"LEER - WAV LADEN":wavNames[i];
    pendingCustom_.swap(pendingCustom);
    pendingLabels_.swap(pendingLabels);
  }
  void commit(Names& liveCustom,Names& liveLabels) noexcept {
    static_assert(std::is_nothrow_swappable_v<Names>);
    liveCustom.swap(pendingCustom_);
    liveLabels.swap(pendingLabels_);
  }
 private:
  Names pendingCustom_{};
  Names pendingLabels_{};
};
}
