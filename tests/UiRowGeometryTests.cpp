#include "core/UiRowGeometry.h"
#include <iostream>
using drumator::UiRowGeometry;
#define CHECK(expr) do { if(!(expr)){std::cerr<<__LINE__<<": " #expr "\n";return __LINE__;}}while(false)
int main() {
 for(std::size_t rows : {std::size_t{4},std::size_t{8},std::size_t{16}}) {
   for(bool expanded : {false,true}) {
     CHECK(UiRowGeometry::gridBottom(rows,expanded)==108.+rows*(expanded?56.:29.));
     for(std::size_t i=0;i<rows;++i) {
       const auto y=UiRowGeometry::rowTop(i,expanded);
       CHECK(UiRowGeometry::rowAt(y,rows,expanded)==i);
       CHECK(UiRowGeometry::mainHit(y+28.99,i,expanded));
       CHECK(!UiRowGeometry::mainHit(y+29.,i,expanded));
       CHECK(UiRowGeometry::rowAt(y+29.,rows,expanded)==(expanded?i:i+1));
       if(expanded) {
         CHECK(UiRowGeometry::velocityHit(622.,y+29.,i));
         CHECK(UiRowGeometry::velocityHit(1229.99,y+55.99,i));
         CHECK(!UiRowGeometry::velocityHit(622.,y+56.,i));
         CHECK(UiRowGeometry::velocityAt(y+31.,i)==127);
         CHECK(UiRowGeometry::velocityAt(y+54.,i)==1);
         CHECK(UiRowGeometry::velocityAt(y+40.,i,true)==0);
       }
     }
     CHECK(UiRowGeometry::rowAt(UiRowGeometry::gridBottom(rows,expanded),rows,expanded)==rows);
   }
 }
 CHECK(UiRowGeometry::stepAt(621.99)==-1);
 CHECK(UiRowGeometry::stepAt(622.)==0);
 CHECK(UiRowGeometry::stepAt(622.+16.*19.)==16);
 CHECK(UiRowGeometry::stepAt(1229.99)==31);
 CHECK(UiRowGeometry::stepAt(1230.)==-1);
 return 0;
}
