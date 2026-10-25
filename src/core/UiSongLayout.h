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
 static constexpr bool canNext(std::size_t page,std::size_t count) noexcept {
   return page+1<pages&&count>=first(page+1);
 }
};
} // namespace drumator
