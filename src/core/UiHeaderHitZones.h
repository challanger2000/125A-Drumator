#pragma once
namespace drumator {
// Consistent header coordinates for drawing and hit testing.
struct UiHeaderHitZones final {
 static constexpr bool masterMinus(double x,double y) noexcept {
   return x>=1140.0&&x<1180.0&&y>=74.0&&y<98.0;
 }
 static constexpr bool masterPlus(double x,double y) noexcept {
   return x>=1184.0&&x<1228.0&&y>=74.0&&y<98.0;
 }
 static constexpr bool songToggle(double x,double y) noexcept {
   return x>=540.0&&x<638.0&&y>=74.0&&y<98.0;
 }
 static constexpr bool songEditor(double x,double y) noexcept {
   return x>=652.0&&x<850.0&&y>=74.0&&y<98.0;
 }
};
}
