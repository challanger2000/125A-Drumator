#pragma once
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <new>
#include <vector>
namespace drumator {
// Preallocated sample-offset timeline. Last source point wins; host wins MIDI ties.
class PatternEventTimeline final {
public:
 struct Point {std::uint8_t pattern{0};std::uint8_t source{0};};
 static constexpr std::uint8_t kMidi=1,kHost=2;
 bool configure(std::size_t frames) {
   try {points_.assign(frames,Point{});}
   catch(const std::bad_alloc&) {points_.clear();return false;}
   length_=0;return true;
 }
 bool reset(std::size_t frames) noexcept {
   if(frames>points_.size())return false;
   std::fill_n(points_.begin(),frames,Point{});
   length_=frames;return true;
 }
 void insert(int offset,unsigned pattern,bool host) noexcept {
   if(offset<0||static_cast<std::size_t>(offset)>=length_||pattern>=8)return;
   auto& point=points_[static_cast<std::size_t>(offset)];
   if(host||point.source!=kHost)
     point={static_cast<std::uint8_t>(pattern),static_cast<std::uint8_t>(host?kHost:kMidi)};
 }
 Point get(std::size_t offset) const noexcept {
   return offset<length_?points_[offset]:Point{};
 }
private:
 std::vector<Point> points_{};
 std::size_t length_{0};
};
}
