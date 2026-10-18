#pragma once
#include "RowFill.h"
#include <array>
namespace drumator {
using PatternSteps=std::array<std::uint8_t,kRows*kSteps>;
constexpr PatternSteps copyPattern(const PatternSteps& p) noexcept {return p;}
constexpr PatternSteps clearPattern() noexcept {return {};}
constexpr PatternSteps fillInstrument(PatternSteps p,std::size_t row,RowFill mode) noexcept {
 if(row>=kRows)return p;
 auto values=filledRow(mode);
 for(std::size_t s=0;s<kSteps;++s)p[row*kSteps+s]=values[s];
 return p;
}
constexpr PatternSteps shiftInstrument(PatternSteps p,std::size_t row,bool right) noexcept {
 if(row>=kRows)return p;
 auto old=p;
 for(std::size_t s=0;s<kSteps;++s)p[row*kSteps+(right?(s+1)%kSteps:(s+kSteps-1)%kSteps)]=old[row*kSteps+s];
 return p;
}
}
