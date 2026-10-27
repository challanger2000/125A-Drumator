#pragma once
#include <cstddef>
#include "core/SongChain.h"

namespace drumator {
// One geometry contract shared by painting and pointer hit testing.
// Shows 16 meaningful song entries at a time instead of 64 tiny buttons.
struct UiSongLayout final {
 static constexpr std::size_t columns=8, perPage=16, pages=SongChain::kCapacity/perPage;
 static constexpr double left=16., top=207., columnPitch=154., rowPitch=52.;
 static constexpr double cellWidth=150.,cellHeight=45.;
 static constexpr double navTop=166.,navBottom=196.;
 static constexpr double previousLeft=988.,previousRight=1102.;
 static constexpr double nextLeft=1110.,nextRight=1244.;
 // Shared rendering/hit geometry for the global SONG START header cluster.
 static constexpr double startTop=169.,startBottom=194.;
 static constexpr double startMinusLeft=438.,startMinusRight=472.;
 static constexpr double startBarLeft=479.,startBarRight=584.;
 static constexpr double startPlusLeft=591.,startPlusRight=625.;
 static constexpr double startResetLeft=632.,startResetRight=717.;
 // A-H and REST form one row; REST is a sentinel, not a pattern.
 static constexpr double choiceTop=339.,choiceBottom=365.;
 static constexpr double patternLeft=115.,patternPitch=46.,patternWidth=42.;
 static constexpr double restLeft=484.,restRight=568.;
 static constexpr double lengthTitleLeft=582.,lengthTitleRight=673.;
 static_assert(startMinusRight<startBarLeft&&startBarRight<startPlusLeft&&
               startPlusRight<startResetLeft&&startResetRight<previousLeft);
 static_assert(patternLeft+7.*patternPitch+patternWidth<restLeft&&
               restRight<lengthTitleLeft&&lengthTitleRight<678.);
 static constexpr bool startMinusHit(double x,double y) noexcept {
   return x>=startMinusLeft&&x<startMinusRight&&y>=startTop&&y<startBottom;
 }
 static constexpr bool startPlusHit(double x,double y) noexcept {
   return x>=startPlusLeft&&x<startPlusRight&&y>=startTop&&y<startBottom;
 }
 static constexpr bool startResetHit(double x,double y) noexcept {
   return x>=startResetLeft&&x<startResetRight&&y>=startTop&&y<startBottom;
 }
 static constexpr int patternChoiceAt(double x,double y) noexcept {
   if(y<choiceTop||y>=choiceBottom)return -1;
   if(x>=restLeft&&x<restRight)return static_cast<int>(SongChain::kRest);
   if(x<patternLeft||x>=patternLeft+8.*patternPitch)return -1;
   const auto index=static_cast<int>((x-patternLeft)/patternPitch);
   return index<8&&x<patternLeft+index*patternPitch+patternWidth?index:-1;
 }
 static constexpr std::size_t first(std::size_t page) noexcept {return page*perPage;}
 static constexpr std::size_t pageOf(std::size_t slot) noexcept {return slot/perPage;}
 static constexpr double cellX(std::size_t column) noexcept {
   return left+static_cast<double>(column)*columnPitch;
 }
 static constexpr double cellY(std::size_t row) noexcept {
   return top+static_cast<double>(row)*rowPitch;
 }
 static constexpr bool previousHit(double x,double y) noexcept {
   return x>=previousLeft&&x<previousRight&&y>=navTop&&y<navBottom;
 }
 static constexpr bool nextHit(double x,double y) noexcept {
   return x>=nextLeft&&x<nextRight&&y>=navTop&&y<navBottom;
 }
 // Sentinel is never a valid slot. Gaps are not clickable.
 static constexpr std::size_t slotAt(double x,double y,std::size_t page) noexcept {
   if(page>=pages||x<left||y<top)return SongChain::kCapacity;
   const auto column=static_cast<std::size_t>((x-left)/columnPitch);
   const auto row=static_cast<std::size_t>((y-top)/rowPitch);
   if(column>=columns||row>=perPage/columns||
      x>=cellX(column)+cellWidth||y>=cellY(row)+cellHeight)
      return SongChain::kCapacity;
   return first(page)+row*columns+column;
 }
 static constexpr bool addHit(double x,double y) noexcept {
   return x>=1012.&&x<1110.&&y>=choiceTop&&y<choiceBottom;
 }
 static constexpr bool removeLastHit(double x,double y) noexcept {
   return x>=1118.&&x<1239.&&y>=choiceTop&&y<choiceBottom;
 }
 static constexpr bool canNext(std::size_t page,std::size_t count) noexcept {
   return page+1<pages&&count>first(page+1);
 }
};
} // namespace drumator
