#pragma once
#include <algorithm>
namespace drumator {
enum class RowControl { Volume, Pan, Pitch };
constexpr int rowControlDefault(RowControl c) noexcept {return c==RowControl::Volume?100:0;}
constexpr int rowControlMin(RowControl c) noexcept {return c==RowControl::Volume?0:c==RowControl::Pan?-100:-24;}
constexpr int rowControlMax(RowControl c) noexcept {return c==RowControl::Volume?200:c==RowControl::Pan?100:24;}
constexpr int rowControlNext(RowControl c,int current,int direction,bool reset=false) noexcept {
 if(reset)return rowControlDefault(c);
 const int step=c==RowControl::Pitch?1:10;
 return std::clamp(current+(direction<0?-step:step),rowControlMin(c),rowControlMax(c));
}
}
