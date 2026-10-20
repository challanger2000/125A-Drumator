#include "core/UiDetailGeometry.h"
#include <iostream>
#include <initializer_list>
using drumator::UiDetailGeometry;
#define CHECK(x) do {if(!(x)){std::cerr<<__LINE__<<": " #x "\n";return __LINE__;}}while(false)
int main(){
 for(const auto gridBottom : {224.,340.,572.,556.}) {
   CHECK(UiDetailGeometry::top(gridBottom)==gridBottom+70.);
   CHECK(UiDetailGeometry::controlAt(10.,gridBottom+110.,gridBottom)==-1);
   for(int i=0;i<5;++i) {
     const auto slot=UiDetailGeometry::slots[static_cast<std::size_t>(i)];
     CHECK(slot.right>slot.left+130.);
     if(i>0)CHECK(slot.left>UiDetailGeometry::slots[static_cast<std::size_t>(i-1)].right);
     CHECK(UiDetailGeometry::controlAt(slot.left+10.,gridBottom+95.,gridBottom)==i);
     CHECK(UiDetailGeometry::controlAt(slot.left+10.,gridBottom+93.,gridBottom)==-1);
     CHECK(UiDetailGeometry::controlAt(slot.right-5.,gridBottom+110.,gridBottom)==-1);
     CHECK(UiDetailGeometry::actionAt(slot.left+10.,i)==UiDetailGeometry::Action::Decrement);
     CHECK(UiDetailGeometry::actionAt(slot.right-10.,i)==UiDetailGeometry::Action::Increment);
     const auto center=(slot.left+slot.right)/2.;
     CHECK(UiDetailGeometry::actionAt(center,i)==UiDetailGeometry::Action::Absolute);
     CHECK(UiDetailGeometry::absoluteAt(slot.left+46.,i,-24,24)==-24);
     CHECK(UiDetailGeometry::absoluteAt(slot.right-46.,i,-24,24)==24);
     CHECK(UiDetailGeometry::absoluteAt(center,i,-24,24)==0);
     CHECK(UiDetailGeometry::absoluteAt(center,i,0,200)==100);
   }
 }
 return 0;
}
