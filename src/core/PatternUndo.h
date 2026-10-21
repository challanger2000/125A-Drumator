#pragma once
#include "PatternOperations.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace drumator {
// Session-local, one-level history for each A-H pattern.
class PatternUndo final {
public:
 // Store the previous duration only for transactions changing 1T/2T.
 bool capture(std::size_t p,const PatternSteps& before,const PatternSteps& after,
              std::uint8_t beforeLength=0,std::uint8_t afterLength=0) noexcept {
  const bool lengthChanged=(beforeLength==16||beforeLength==32)&&
      (afterLength==16||afterLength==32)&&beforeLength!=afterLength;
  if(p>=kPatterns||(before==after&&!lengthChanged))return false;
  previous_[p]=before;
  previousLength_[p]=lengthChanged?beforeLength:0;
  valid_[p]=true;return true;
 }
 std::uint8_t snapshotLength(std::size_t p) const noexcept {
  return available(p)?previousLength_[p]:0;
 }
 bool available(std::size_t p) const noexcept {return p<kPatterns&&valid_[p];}
 const PatternSteps* snapshot(std::size_t p) const noexcept {
  return available(p)?&previous_[p]:nullptr;
 }
 void consume(std::size_t p) noexcept {if(p<kPatterns)valid_[p]=false;}
 void reset() noexcept {valid_.fill(false);}
private:
 std::array<PatternSteps,kPatterns> previous_{};
 std::array<std::uint8_t,kPatterns> previousLength_{};
 std::array<bool,kPatterns> valid_{};
};
} // namespace drumator
