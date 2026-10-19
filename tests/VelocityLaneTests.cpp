#include "core/VelocityLane.h"
#include <cassert>
using drumator::VelocityLane;
int main() {
 constexpr double bottom=224.0;
 static_assert(VelocityLane::kBarCount==32);
 assert(VelocityLane::stepAt(621.99)==-1);
 assert(VelocityLane::stepAt(622.0)==0);
 assert(VelocityLane::stepAt(622.0+19.*15.+9.)==15);
 assert(VelocityLane::stepAt(622.0+19.*31.+18.)==31);
 assert(VelocityLane::stepAt(1230.0)==-1);
 assert(VelocityLane::valueAt(VelocityLane::barTop(bottom),bottom)==127);
 assert(VelocityLane::valueAt(VelocityLane::barBottom(bottom),bottom)==1);
 assert(VelocityLane::valueAt(-10000.0,bottom)==127);
 assert(VelocityLane::valueAt(10000.0,bottom)==1);
 assert(VelocityLane::valueAt(VelocityLane::barTop(bottom),bottom,true)==0);
 assert(VelocityLane::heightFor(0)==0.0);
 assert(VelocityLane::heightFor(127)==VelocityLane::kBarHeight);
 for(int i=0;i<32;++i) {
   const auto x=VelocityLane::kLeft+(static_cast<double>(i)+.5)*VelocityLane::kCellWidth;
   assert(VelocityLane::stepAt(x)==i);
   assert(VelocityLane::hit(x,VelocityLane::barTop(bottom),bottom));
   assert(VelocityLane::hit(x,VelocityLane::barBottom(bottom),bottom));
   assert(!VelocityLane::hit(x,VelocityLane::barBottom(bottom)+.1,bottom));
 }
 return 0;
}
