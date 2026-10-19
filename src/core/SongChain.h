#pragma once
#include "SequencerCore.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace drumator {
// Deterministic chain coordinates in sixteenth-note steps. No audio callback
// allocations, cumulative block drift, or dependence on playback direction.
class SongChain final {
public:
 static constexpr std::size_t kCapacity=64;
 struct Entry {std::uint8_t pattern{0},steps{32};};
 struct Position {std::uint8_t pattern{0},step{0};std::size_t entry{0};};
 constexpr SongChain() noexcept {
  for(std::size_t i=0;i<kPatterns;++i) entries_[i]={static_cast<std::uint8_t>(i),32};
 }
 constexpr bool setCount(std::size_t count) noexcept {
  if(count==0||count>kCapacity)return false;
  for(std::size_t i=count_;i<count;++i) entries_[i]={0,32};
  count_=count;return true;
 }
 constexpr bool configure(std::size_t index,std::uint8_t pattern,std::uint8_t steps) noexcept {
  if(index>=count_||pattern>=kPatterns||!validLength(steps))return false;
  entries_[index]={pattern,steps};return true;
 }
 static constexpr bool validLength(std::uint8_t steps) noexcept {
  return steps==8||steps==16||steps==32;
 }
 constexpr std::size_t count() const noexcept {return count_;}
 constexpr Entry entry(std::size_t i) const noexcept {return i<count_?entries_[i]:Entry{};}
 constexpr std::uint64_t totalSteps() const noexcept {
  std::uint64_t sum=0;
  for(std::size_t i=0;i<count_;++i)sum+=entries_[i].steps;
  return sum;
 }
 constexpr Position at(std::int64_t absoluteStep,std::int64_t originStep=0) const noexcept {
  const auto total=static_cast<std::int64_t>(totalSteps());
  if(total==0)return {};
  const auto phase=absoluteStep%total,origin=originStep%total;
  auto offset=(phase-origin+total)%total;
  for(std::size_t i=0;i<count_;++i) {
   if(offset<entries_[i].steps)
    return {entries_[i].pattern,static_cast<std::uint8_t>(offset),i};
   offset-=entries_[i].steps;
  }
  return {};
 }
private:
 std::array<Entry,kCapacity> entries_{};
 std::size_t count_{kPatterns};
};
}
