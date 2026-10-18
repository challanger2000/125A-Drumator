#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
namespace drumator {
// Pure, deterministic selection. The caller validates and loads the returned file.
inline bool adjacentSample(const std::vector<std::string>& paths,
                           const std::string& current,int direction,
                           std::string& next) {
 if((direction!=1&&direction!=-1)||paths.size()<2)return false;
 std::vector<std::string> ordered=paths;
 std::sort(ordered.begin(),ordered.end());
 ordered.erase(std::unique(ordered.begin(),ordered.end()),ordered.end());
 if(ordered.size()<2)return false;
 const auto it=std::lower_bound(ordered.begin(),ordered.end(),current);
 if(it==ordered.end()||*it!=current)return false;
 const auto pos=static_cast<std::size_t>(it-ordered.begin());
 const auto destination=direction>0?(pos+1)%ordered.size():
                            (pos+ordered.size()-1)%ordered.size();
 next=ordered[destination];return true;
}
}
