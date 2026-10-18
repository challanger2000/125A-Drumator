#include "core/AtomicPatternBank.h"
#include "core/StepVelocity.h"
#include "core/RowFill.h"
#include "core/RowMixControl.h"
#include "core/PatternOperations.h"
#include <array>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace drumator;
static void check(bool okay,const char* msg){if(!okay)throw std::runtime_error(msg);}
int main(){
 try {
  for(auto mode:{RowFill::Every2,RowFill::Every4,RowFill::Every8,RowFill::All,RowFill::Clear}) {
    const auto row=filledRow(mode);
    const auto spacing=mode==RowFill::Every2?2u:mode==RowFill::Every4?4u:mode==RowFill::Every8?8u:1u;
    std::size_t active=0;
    for(std::size_t step=0;step<kSteps;++step) {
      const auto expected=mode!=RowFill::Clear&&step%spacing==0?127:0;
      check(row[step]==expected,"row fill positions");
      if(row[step])++active;
    }
    check(active==(mode==RowFill::Clear?0u:kSteps/spacing),"row fill count");
  }
  check(filledRow(RowFill::Every4,96)[0]==96&&filledRow(RowFill::Every4,96)[1]==0,
        "fill preserves requested velocity");
  PatternSteps example{};
  example[0]=93; example[31]=45; example[kSteps+7]=64;
  auto filled=fillInstrument(example,0,RowFill::Every8);
  check(filled[0]==127&&filled[8]==127&&filled[31]==0&&filled[kSteps+7]==64,"row fill isolation");
  auto moved=shiftInstrument(example,0,true);
  check(moved[1]==93&&moved[0]==45&&shiftInstrument(moved,0,false)==example,"shift wrap roundtrip");
  check(copyPattern(example)==example&&clearPattern()==PatternSteps{},"pattern copy clear");
  for(auto control:{RowControl::Volume,RowControl::Pan,RowControl::Pitch}) {
    const int lo=rowControlMin(control),hi=rowControlMax(control),center=rowControlDefault(control);
    check(rowControlNext(control,hi,-1)<hi,"cannot leave maximum");
    check(rowControlNext(control,lo,1)>lo,"cannot leave minimum");
    check(rowControlNext(control,hi,1)==hi&&rowControlNext(control,lo,-1)==lo,"control limits");
    check(rowControlNext(control,hi,1,true)==center&&rowControlNext(control,lo,-1,true)==center,"control reset");
  }
  check(emptyPattern(PatternSteps{}),"empty pattern detection");
  PatternSteps populated{};populated[kSteps+4]=92;
  check(!emptyPattern(populated),"nonempty source detection");
  std::array<PatternSteps,kPatterns> slots{};
  slots[0][0]=127;slots[1][0]=80;slots[3][5]=64;
  check(nextEmptyPattern(0,[&](std::size_t i){return slots[i];})==2,
        "duplicate must skip occupied destination");
  slots[7][0]=99;
  check(nextEmptyPattern(7,[&](std::size_t i){return slots[i];})==2,
        "duplicate must wrap around");
  for(auto& slot:slots)slot[0]=127;
  check(nextEmptyPattern(0,[&](std::size_t i){return slots[i];})==kPatterns,
        "no free destination must not overwrite");
  AtomicPatternBank bank;
  check(nextStepVelocity(0)==32&&nextStepVelocity(32)==64&&
        nextStepVelocity(64)==96&&nextStepVelocity(96)==127&&
        nextStepVelocity(127)==0,"velocity cycle sequence");
  check(nextStepVelocity(20)==32&&nextStepVelocity(80)==96&&
        nextStepVelocity(115)==127,"velocity cycle from loaded projects");
  check(bank.get(0,0,0)==127&&bank.get(7,0,4)==90,
        "old factory fixture default changed");
  check(bank.get(7,15,31)==0,"last step not silent");
  check(bank.set(7,15,31,127),"write last cell");
  check(bank.get(7,15,31)==127,"last cell not readable");
  check(!bank.set(8,0,0,1)&&!bank.set(0,16,0,1)&&
        !bank.set(0,0,32,1)&&!bank.set(0,0,0,128),
        "invalid cell accepted");
  check(bank.select(7)&&bank.selected()==7,"pattern change");
  PatternBank::State stored=bank.snapshot();
  check(stored[5]==7,"active pattern not stored");
  AtomicPatternBank restored;
  check(restored.restore(stored.data(),stored.size()),"restore failed");
  check(restored.snapshot()==stored,"atomic bank roundtrip differs");
  auto invalid=stored;invalid.back()=255;
  check(!restored.restore(invalid.data(),invalid.size()),"invalid grid accepted");
  check(restored.snapshot()==stored,"invalid restore changed live grid");
  // Simulate control and realtime readers with guaranteed atomic access.
  std::atomic<bool> stop{false};
  std::thread writer([&]{
    for(unsigned i=0;i<40000;++i)bank.set(0,1,3,i%128);
    stop.store(true,std::memory_order_release);
  });
  std::uint64_t samples=0;
  while(!stop.load(std::memory_order_acquire)){
    const auto v=bank.get(0,1,3);
    check(v<=127,"read torn cell");
    samples+=v;
  }
  writer.join();
  (void)samples;
  // Audit B01: concurrent full-pattern replacement must never expose
  // a hybrid of two generations to the scheduler's coherent reader.
  AtomicPatternBank transactionBank;
  std::array<std::uint8_t,kRows*kSteps> oldPattern{},newPattern{},coherent{};
  oldPattern.fill(15);newPattern.fill(115);
  check(transactionBank.replacePattern(0,oldPattern.data(),oldPattern.size()),"initial transaction");
  std::atomic<bool> replaceDone{false};
  std::thread replacer([&]{
    for(int iteration=0;iteration<2000;++iteration){
      const auto& source=(iteration&1)?oldPattern:newPattern;
      if(!transactionBank.replacePattern(0,source.data(),source.size()))
        std::terminate();
    }
    replaceDone.store(true,std::memory_order_release);
  });
  std::size_t verified=0;
  while(!replaceDone.load(std::memory_order_acquire)){
    if(transactionBank.coherentPattern(0,coherent)){
      check(coherent==oldPattern||coherent==newPattern,
            "torn pattern generation during replacement");
      struct StablePattern {
        const std::array<std::uint8_t,kRows*kSteps>& data;
        std::uint8_t selected() const noexcept {return 0;}
        std::uint8_t get(std::size_t p,std::size_t row,std::size_t step) const noexcept {
          return p==0?data[row*kSteps+step]:0;
        }
      } stable{coherent};
      unsigned events=0;
      const auto scheduled=StepScheduler::run(stable,Clock{0,120,48000,256,true},
          [&](Trigger hit) {
            check(hit.velocity==15||hit.velocity==115,"mixed velocity generation");
            ++events;
          });
      check(scheduled.status==ScheduleStatus::Ok&&events==kRows,
            "coherent scheduler lost events");
      ++verified;
    }
  }
  replacer.join();
  check(transactionBank.coherentPattern(0,coherent),"final coherent generation");
  check(coherent==oldPattern||coherent==newPattern,"invalid final generation");
  check(!transactionBank.replacePattern(0,newPattern.data(),newPattern.size()-1),
        "invalid replacement length accepted");
  (void)verified;
  std::cout<<"Atomic pattern bank: PASS\n";
  return 0;
 }catch(const std::exception& ex){
  std::cerr<<"Atomic pattern bank: FAIL "<<ex.what()<<'\n';
  return 1;
 }
}
