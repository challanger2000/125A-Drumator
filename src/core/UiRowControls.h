#pragma once
namespace drumator {
// Instrument row coordinates, shared by renderer, mouse and name-editor.
// Horizontal hit bounds are half open; gaps between controls are intentional.
struct UiRowControls final {
 struct Slot {double left,right;};
 static constexpr Slot fill{16.,56.};
 static constexpr Slot preview{61.,95.};
 static constexpr Slot sample{102.,266.};
 static constexpr Slot previous{268.,292.};
 static constexpr Slot next{294.,318.};
 static constexpr Slot output{322.,378.};
 static constexpr Slot volume{384.,445.};
 static constexpr Slot pan{450.,510.};
 static constexpr Slot pitch{515.,565.};
 static constexpr Slot mute{569.,592.};
 static constexpr Slot solo{596.,619.};
 static constexpr bool hit(Slot r,double x) noexcept {return x>=r.left&&x<r.right;}
 static constexpr double mid(Slot r) noexcept {return (r.left+r.right)/2.;}
};
}
