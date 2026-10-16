// 125A Drumator: UI visibility only; engine retains 16 fixed row slots.
// Removal means "hide final EMPTY row", never delete samples or step state.
#pragma once
#include "SequencerCore.h"
#include <cstddef>
#include <cstdint>
namespace drumator {
constexpr std::uint8_t kDefaultVisibleRows=4;
constexpr std::uint32_t kStateMagicV3=0x33524D44u; // "DMR3"
class RowVisibility {
public:
 static bool valid(std::size_t n) noexcept {return n>=1&&n<=kRows;}
 template<class Grid>
 static bool canHideLast(std::size_t count,const Grid& bank,
                         bool hasLoadedSample) noexcept {
   if(count<=1||count>kRows||hasLoadedSample)return false;
   const auto row=count-1;
   for(std::size_t p=0;p<kPatterns;++p)
      for(std::size_t s=0;s<kSteps;++s)
         if(bank.get(p,row,s)!=0)return false;
   return true;
 }
};
}
