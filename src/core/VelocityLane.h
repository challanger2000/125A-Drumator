// Lightweight 32-bar velocity lane hit mapping, independent from VSTGUI.
#pragma once
#include <cstddef>
#include <cstdint>
namespace drumator {
struct VelocityLane final {
 static constexpr std::size_t kBarCount=32;
 static constexpr double kLeft=622.0;
 static constexpr double kCellWidth=19.0;
 static constexpr double kWidth=kCellWidth*kBarCount;
 static constexpr double kPanelOffset=77.0; // relative to grid bottom
 static constexpr double kBarTopOffset=18.0; // relative to lane panel top
 static constexpr double kBarHeight=70.0;
 static constexpr double kFooterHeight=220.0;
 static constexpr double kClosedFooterHeight=105.0;
 static constexpr double panelTop(double gridBottom) noexcept {
   return gridBottom+kPanelOffset;
 }
 static constexpr double barTop(double gridBottom) noexcept {
   return panelTop(gridBottom)+kBarTopOffset;
 }
 static constexpr double barBottom(double gridBottom) noexcept {
   return barTop(gridBottom)+kBarHeight;
 }
 static constexpr int stepAt(double x) noexcept {
   return x>=kLeft&&x<kLeft+kWidth?
     static_cast<int>((x-kLeft)/kCellWidth):-1;
 }
 static constexpr bool hit(double x,double y,double gridBottom) noexcept {
   return stepAt(x)>=0&&y>=barTop(gridBottom)&&y<=barBottom(gridBottom);
 }
 static constexpr std::uint8_t valueAt(double y,double gridBottom,bool erase=false) noexcept {
   if(erase)return 0;
   const auto top=barTop(gridBottom),bottom=barBottom(gridBottom);
   const auto clamped=y<top?top:y>bottom?bottom:y;
   const auto value=static_cast<int>((bottom-clamped)*126.0/kBarHeight+0.5)+1;
   return static_cast<std::uint8_t>(value);
 }
 static constexpr double heightFor(std::uint8_t velocity) noexcept {
   return kBarHeight*static_cast<double>(velocity)/127.0;
 }
};
} // namespace drumator
