#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
#include <cmath>
namespace drumator {
// One authoritative layout for the selected-instrument precision controls.
// Draw and hit testing must share these logical VSTGUI coordinates.
struct UiDetailGeometry final {
 struct Slot {double left,right;};
 enum class Action {None,Decrement,Absolute,Increment};
 static constexpr double topOffset=70.0;
 static constexpr double panelHeight=64.0;
 static constexpr double footerHeight=174.0;
 static constexpr std::array<Slot,5> slots{{{235.,419.},{432.,616.},{629.,813.},
                                           {826.,1010.},{1023.,1227.}}};
 static constexpr double top(double gridBottom) noexcept {return gridBottom+topOffset;}
 static constexpr int controlAt(double x,double y,double gridBottom) noexcept {
   const auto t=top(gridBottom);
   if(!(y>=t+24.&&y<t+61.))return -1;
   for(std::size_t i=0;i<slots.size();++i)
     if(x>=slots[i].left+5.&&x<slots[i].right-5.)
       return static_cast<int>(i);
   return -1;
 }
 static constexpr Action actionAt(double x,int index) noexcept {
   if(index<0||index>=static_cast<int>(slots.size()))return Action::None;
   const auto slot=slots[static_cast<std::size_t>(index)];
   if(x<slot.left+44.)return Action::Decrement;
   if(x>=slot.right-44.)return Action::Increment;
   return Action::Absolute;
 }
 static int absoluteAt(double x,int index,int minimum,int maximum) noexcept {
   if(index<0||index>=static_cast<int>(slots.size())||maximum<=minimum)return minimum;
   const auto slot=slots[static_cast<std::size_t>(index)];
   const double left=slot.left+46.,right=slot.right-46.;
   const auto fraction=std::clamp((x-left)/(right-left),0.0,1.0);
   return std::clamp(static_cast<int>(std::lround(minimum+
           fraction*static_cast<double>(maximum-minimum))),minimum,maximum);
 }
};
}
