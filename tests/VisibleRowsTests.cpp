#include "core/VisibleRows.h"
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
   std::cout<<"Visible-row UX preservation checks PASS\n";
   return 0;
 } catch(const std::exception& e){
   std::cerr<<"Visible-row UX tests FAIL: "<<e.what()<<'\n';
   return 1;
 }
}
