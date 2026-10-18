#include "core/VisibleRows.h"
#include "core/RowResizeTransaction.h"
#include "core/HostEditDirty.h"
#include "core/PatternQuantization.h"
#include "core/AtomicPatternBank.h"
#include <iostream>
#include <stdexcept>
using namespace drumator;
static void check(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
int main(){
 try {
   AtomicPatternBank grid;
   check(kDefaultVisibleRows==4,"Initial four rows changed");
   check(!RowVisibility::valid(0)&&!RowVisibility::valid(17),"range guard");
   check(RowVisibility::valid(1)&&RowVisibility::valid(4)&&
         RowVisibility::valid(16),"valid range");
   // Factory content on row zero: we may hide row 4 safely, never row one.
   check(RowVisibility::canHideLast(4,grid,false),"empty fourth row");
   check(!RowVisibility::canHideLast(1,grid,false),"cannot hide last visible row");
   check(!RowVisibility::canHideLast(4,grid,true),"loaded sample must block hide");
   check(grid.set(7,3,31,90),"last step");
   check(!RowVisibility::canHideLast(4,grid,false),
         "step in another pattern must block hide");
   check(grid.set(7,3,31,0),"clear last step");
   check(RowVisibility::canHideLast(4,grid,false),"unlocked empty row");
   check(!RowVisibility::canHideLast(17,grid,false),"invalid row bound");
   check(RowVisibility::canHideLast(16,grid,false),"last row empty");
   check(!RowVisibility::canHideLast(2,grid,true),"second row with audio");
   // F10: mock host accepts/rejects window changes; no invisible row
   // count changes may survive a rejected resize.
   int rows=4;
   auto add=[&](){++rows;return true;};
   auto subtract=[&](){--rows;return true;};
   check(tryRowResizeTransaction(add,[]{return false;},subtract)==
         RowResizeResult::Reverted&&rows==4,
         "F10 rejected host resize left an invisible instrument");
   check(tryRowResizeTransaction(add,[]{return true;},subtract)==
         RowResizeResult::Applied&&rows==5,
         "F10 accepted resize failed to keep row");
   check(tryRowResizeTransaction(subtract,[]{return false;},add)==
         RowResizeResult::Reverted&&rows==5,
         "F10 rejected shrink incorrectly removed row");
   check(tryRowResizeTransaction([]{return false;},[]{return true;},[]{return true;})==
         RowResizeResult::EditRejected,"F10 edit rejection not preserved");
   std::cout<<"F10 resize reject/rollback regression PASS\\n";
   int dirtyCalls=0;
   markSuccessfulHostEdit(false,[&](){++dirtyCalls;});
   check(dirtyCalls==0,"F05 rejected edit dirtied host project");
   markSuccessfulHostEdit(true,[&](){++dirtyCalls;});
   check(dirtyCalls==1,"F05 accepted edit did not dirty host project");
   std::cout<<"F05 successful-edit dirty notification policy PASS\\n";
   for(int i=0;i<=1000;++i) {
     const double n=static_cast<double>(i)/1000.;
     check(patternFromNormalized(n)==
           static_cast<unsigned>(std::min(7.0,std::floor(n*8.))),
           "N05 quantization mismatch");
   }
   std::cout<<"N05 1001 discrete pattern values PASS\\n";
   std::cout<<"Visible-row UX preservation checks PASS\n";
   return 0;
 } catch(const std::exception& e){
   std::cerr<<"Visible-row UX tests FAIL: "<<e.what()<<'\n';
   return 1;
 }
}
