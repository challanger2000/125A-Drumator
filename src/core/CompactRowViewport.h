#pragma once
#include <algorithm>
#include <cstddef>
namespace drumator {
// Row paging for a host that refuses to enlarge the plugin editor.
struct CompactRowViewport {
 static constexpr std::size_t pageSize=4;
 static std::size_t first(std::size_t total,std::size_t requested) noexcept {
   return total==0?0:std::min(requested,total-1);
 }
 static std::size_t count(std::size_t total,std::size_t start) noexcept {
   return total==0?0:std::min(pageSize,total-first(total,start));
 }
 static std::size_t previous(std::size_t total,std::size_t start) noexcept {
   const auto current=first(total,start);
   return current>=pageSize?current-pageSize:0;
 }
 static std::size_t next(std::size_t total,std::size_t start) noexcept {
   const auto current=first(total,start);
   return current+pageSize<total?current+pageSize:current;
 }
 static std::size_t row(std::size_t total,std::size_t start,std::size_t index) noexcept {
   return index<count(total,start)?first(total,start)+index:total;
 }
};
}
