#include "core/VelocityLane.h"
#define VERIFY(condition) do { if(!(condition))return __LINE__; } while(false)
using drumator::VelocityLane;
int main() {
 constexpr double bottom=224.0;
 static_assert(VelocityLane::kBarCount==32);
 VERIFY(VelocityLane::stepAt(621.99)==-1);
 VERIFY(VelocityLane::stepAt(622.0)==0);
 VERIFY(VelocityLane::stepAt(622.0+19.*15.+9.)==15);
 VERIFY(VelocityLane::stepAt(622.0+19.*31.+18.)==31);
 VERIFY(VelocityLane::stepAt(1230.0)==-1);
 VERIFY(VelocityLane::valueAt(VelocityLane::barTop(bottom),bottom)==127);
 VERIFY(VelocityLane::valueAt(VelocityLane::barBottom(bottom),bottom)==1);
 VERIFY(VelocityLane::valueAt(-10000.0,bottom)==127);
 VERIFY(VelocityLane::valueAt(10000.0,bottom)==1);
 VERIFY(VelocityLane::valueAt(VelocityLane::barTop(bottom),bottom,true)==0);
 VERIFY(VelocityLane::heightFor(0)==0.0);
 VERIFY(VelocityLane::heightFor(127)==VelocityLane::kBarHeight);
 for(int i=0;i<32;++i) {
   const auto x=VelocityLane::kLeft+(static_cast<double>(i)+.5)*VelocityLane::kCellWidth;
   VERIFY(VelocityLane::stepAt(x)==i);
   VERIFY(VelocityLane::hit(x,VelocityLane::barTop(bottom),bottom));
   VERIFY(VelocityLane::hit(x,VelocityLane::barBottom(bottom),bottom));
   VERIFY(!VelocityLane::hit(x,VelocityLane::barBottom(bottom)+.1,bottom));
 }
 return 0;
}
