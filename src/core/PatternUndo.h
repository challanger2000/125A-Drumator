#pragma once
#include "PatternOperations.h"
#include <array>
#include <cstddef>
namespace drumator {
// Session-local, one-level history for each A-H pattern.
class PatternUndo final {
public:
 bool capture(std::size_t p,const PatternSteps& before,const PatternSteps& after) noexcept {
  if(p>=kPatterns||before==after)return false;
  previous_[p]=before;valid_[p]=true;return true;
 }
 bool available(std::size_t p) const noexcept {return p<kPatterns&&valid_[p];}
 const PatternSteps* snapshot(std::size_t p) const noexcept {
  return available(p)?&previous_[p]:nullptr;
 }
 void consume(std::size_t p) noexcept {if(p<kPatterns)valid_[p]=false;}
 void reset() noexcept {valid_.fill(false);}
private:
 std::array<PatternSteps,kPatterns> previous_{};
 std::array<bool,kPatterns> valid_{};
};
} // namespace drumator
