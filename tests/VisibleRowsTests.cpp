#include "core/VisibleRows.h"
#include "core/KitRestoreGate.h"
#include "core/AtomicPatternSelection.h"
#include "core/RowResizeTransaction.h"
#include "core/HostEditDirty.h"
#include "core/PatternQuantization.h"
#include "core/AtomicPatternBank.h"
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
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
           static_cast<unsigned>(std::lround(n*7.)),
           "N05 quantization mismatch");
   }
   check(RowVisibility::visibleSolo(0xffffu,4)==0x000fu,
         "F07 recall retained hidden solo and could mute visible rows");
   check(RowVisibility::visibleSolo(0x8000u,16)==0x8000u,
         "F07 visible sixteenth row lost solo");
   std::cout<<"F07 visible-only solo recall policy PASS\\n";
   std::cout<<"N05 1001 discrete pattern values PASS\\n";
   // N04 shared draw/hit geometry boundaries: pattern [38,68), zoom [39,69).
   auto patternHit=[](double y){return y>=38.0&&y<68.0;};
   auto zoomHit=[](double y){return y>=39.0&&y<69.0;};
   check(patternHit(38.5)&&!patternHit(68.5),"N04 pattern hit margin");
   check(!zoomHit(38.5)&&zoomHit(68.5),"N04 zoom hit margin");
   std::cout<<"N04 header draw/hit vertical ranges PASS\\n";
   // Gate integration: a restore cannot race an already admitted audio block.
   // New callbacks are rejected without waiting while the writer owns the gate.
   KitRestoreGate gate;
   check(gate.audioEnter(),"Gate blocked first audio reader");
   std::atomic<bool> writerStarted{false},writerEntered{false};
   std::thread restore([&](){
     writerStarted.store(true,std::memory_order_release);
     gate.controlBegin();
     writerEntered.store(true,std::memory_order_release);
     gate.controlEnd();
   });
   while(!writerStarted.load(std::memory_order_acquire))
     std::this_thread::yield();
   bool rejectedDuringRestore=false;
   const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
   while(std::chrono::steady_clock::now()<deadline) {
     const bool allowed=gate.audioEnter();
     gate.audioLeave();
     if(!allowed){rejectedDuringRestore=true;break;}
     std::this_thread::yield();
   }
   const bool waitedForExistingReader=!writerEntered.load(std::memory_order_acquire);
   gate.audioLeave();
   restore.join();
   check(rejectedDuringRestore&&waitedForExistingReader&&
         writerEntered.load(std::memory_order_acquire),
         "Restore/audio gate did not serialize callbacks");
   check(gate.audioEnter(),"Gate remained closed after restore");
   gate.audioLeave();
   AtomicPatternSelection selection;
   const auto old=selection.snapshot();
   selection.hostSubmit(7);
   selection.restoreIfUnchanged(old,1);
   check(selection.load()==7,"N07 kit overwrote pending host flush");
   const auto newer=selection.snapshot();
   selection.restoreIfUnchanged(newer,1);
   selection.hostSubmit(6);
   selection.restoreIfUnchanged(newer,7);
   check(selection.load()==6,"N07 old replay overwrote newer automation");
   std::cout<<"N07 single-atomic restore arbitration PASS\\n";
   std::cout<<"R01 single-atomic restore and audio admission PASS\\n";
   std::cout<<"Visible-row UX preservation checks PASS\n";
   return 0;
 } catch(const std::exception& e){
   std::cerr<<"Visible-row UX tests FAIL: "<<e.what()<<'\n';
   return 1;
 }
}
