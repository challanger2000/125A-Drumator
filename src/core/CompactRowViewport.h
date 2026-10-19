#pragma once
#include <algorithm>
#include <cstddef>
namespace drumator {
// Row paging for a host that refuses to enlarge the plugin editor.
struct CompactRowViewport {
 static constexpr std::size_t pageSize=4;
  // Number of instrument rows whose FULL footer fits the host's real height.
  static std::size_t fittingRows(double client,double gridTop,
                                 double rowPitch,double footer) noexcept {
    if(!(rowPitch>0.0)||client<gridTop+footer+rowPitch)return 0;
    return std::min(pageSize,static_cast<std::size_t>((client-gridTop-footer)/rowPitch));
  }
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
