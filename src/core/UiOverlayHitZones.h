#pragma once
#include <cstddef>
namespace drumator {
struct UiOverlayHitZones {
 static constexpr bool canDrop(double x,double y,std::size_t rows,bool song,bool fill,double top) noexcept {
  return !song&&x>=16.&&x<=1230.&&y>=108.&&y<108.+29.*rows&&
         !(fill&&x>=650.&&x<819.&&y>=top&&y<top+225.);
 }
};
}
