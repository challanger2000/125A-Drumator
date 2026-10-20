#pragma once
#include <cstddef>
#include <cstdint>
namespace drumator {
// One source of truth for rendered rows, hit testing and drag/drop.
struct UiRowGeometry final {
 static constexpr double top=108.0, mainHeight=29.0, velocityHeight=27.0;
 static constexpr double stepX=622.0, stepWidth=19.0;
 static constexpr std::size_t steps=32;
 static constexpr double pitch(bool expanded) noexcept {
   return mainHeight+(expanded?velocityHeight:0.0);
 }
 static constexpr double rowTop(std::size_t index,bool expanded) noexcept {
   return top+static_cast<double>(index)*pitch(expanded);
 }
 static constexpr double gridBottom(std::size_t count,bool expanded) noexcept {
   return rowTop(count,expanded);
 }
 static constexpr std::size_t rowAt(double y,std::size_t count,bool expanded) noexcept {
   return y>=top&&y<gridBottom(count,expanded)?
     static_cast<std::size_t>((y-top)/pitch(expanded)):count;
 }
 static constexpr bool mainHit(double y,std::size_t index,bool expanded) noexcept {
   return y>=rowTop(index,expanded)&&y<rowTop(index,expanded)+mainHeight;
 }
 static constexpr int stepAt(double x) noexcept {
   return x>=stepX&&x<stepX+steps*stepWidth?
     static_cast<int>((x-stepX)/stepWidth):-1;
 }
 static constexpr bool velocityHit(double x,double y,std::size_t index) noexcept {
   return stepAt(x)>=0&&y>=rowTop(index,true)+mainHeight&&
          y<rowTop(index+1,true);
 }
 static constexpr std::uint8_t velocityAt(double y,std::size_t index,bool erase=false) noexcept {
   if(erase)return 0;
   const auto t=rowTop(index,true)+31.0,b=rowTop(index,true)+54.0;
   const auto clamped=y<t?t:y>b?b:y;
   return static_cast<std::uint8_t>(1+static_cast<int>((b-clamped)*126.0/23.0+0.5));
 }
};
}
