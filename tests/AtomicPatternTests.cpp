#include "core/AtomicPatternBank.h"
#include "core/AtomicPatternSelection.h"
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
#include <semaphore>
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
  {
    const auto row=filledRow(RowFill::Snare24);
    for(std::size_t step=0;step<kSteps;++step)
      check(row[step]==((step%8==4)?127:0),"snare 2 and 4 fill");
    check(filledRow(RowFill::Snare24,96)[4]==96,"snare velocity");
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
  // A restored state must expose coherent full-pattern generations.
  std::array<std::uint8_t,kRows*kSteps> restoredGeneration{};
  for(std::size_t pattern=0;pattern<kPatterns;++pattern) {
    check(restored.coherentPattern(pattern,restoredGeneration),
          "restored pattern generation unavailable");
    for(std::size_t row=0;row<kRows;++row)
      for(std::size_t step=0;step<kSteps;++step)
        check(restoredGeneration[row*kSteps+step]==
                  bank.get(pattern,row,step),
              "restored generation differs from source");
  }
  auto invalid=stored;invalid.back()=255;
  check(!restored.restore(invalid.data(),invalid.size()),"invalid grid accepted");
  check(restored.snapshot()==stored,"invalid restore changed live grid");
  // Host automation must outrank kit restore even for ABA/same-value events.
  {
    AtomicPatternSelection selection;
    selection.store(2);
    const auto before=selection.snapshot();
    selection.hostSubmit(2); // same visible selection, new event generation
    selection.restoreIfUnchanged(before,6);
    check(selection.load()==2,"same-value host event overwritten by kit restore");
    const auto aba=selection.snapshot();
    selection.hostSubmit(5);
    selection.hostSubmit(2);
    selection.restoreIfUnchanged(aba,7);
    check(selection.load()==2,"A-B-A host events overwritten by kit restore");
    const auto clean=selection.snapshot();
    selection.restoreIfUnchanged(clean,4);
    check(selection.load()==4,"uncontested kit restore did not select kit pattern");
    const auto contested=selection.snapshot();
    selection.hostSubmit(1);
    selection.restoreIfUnchanged(contested,3);
    check(selection.load()==1,"new host automation lost to kit restore");
    // M37: MIDI selection was once only a low-bit store(). A fast A-B-A
    // sequence restored the original pattern value but not its generation,
    // allowing the in-progress older kit recall to overwrite it.
    selection.store(2);
    const auto midiBefore=selection.snapshot();
    selection.midiSubmit(2); // same-value event is still authoritative
    check(selection.snapshot()!=midiBefore&&selection.load()==2,
          "M37 same-pattern MIDI did not advance generation");
    selection.restoreIfUnchanged(midiBefore,6);
    check(selection.load()==2,"M37 MIDI same-pattern overwritten by recall");
    const auto midiAba=selection.snapshot();
    selection.midiSubmit(5);
    selection.midiSubmit(2);
    check(selection.snapshot()!=midiAba&&selection.load()==2,
          "M37 MIDI A-B-A was invisible to restore arbiter");
    selection.restoreIfUnchanged(midiAba,7);
    check(selection.load()==2,"M37 MIDI A-B-A overwritten by older kit");
    const auto midiCompeting=selection.snapshot();
    selection.midiSubmit(3);
    selection.restoreIfUnchanged(midiCompeting,4);
    check(selection.load()==3,"M37 new MIDI selection lost to kit recall");
    const auto uncontestedMidi=selection.snapshot();
    selection.restoreIfUnchanged(uncontestedMidi,1);
    check(selection.load()==1,"M37 unopposed kit recall stopped working");
  }
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
  // Invalid complete replacements must not partially change a live pattern.
  AtomicPatternBank guarded;
  std::array<std::uint8_t,kRows*kSteps> good{},bad{},readback{};
  good.fill(72);
  bad.fill(43);
  bad.back()=128;
  check(guarded.replacePattern(3,good.data(),good.size()),
        "valid bulk pattern was rejected");
  check(!guarded.replacePattern(3,bad.data(),bad.size()),
        "invalid bulk velocity accepted");
  check(guarded.coherentPattern(3,readback)&&readback==good,
        "invalid bulk replacement partially changed pattern");
  check(!guarded.replacePattern(3,nullptr,good.size()),
        "null pattern payload accepted");
  check(!guarded.replacePattern(kPatterns,good.data(),good.size()),
        "out-of-range destination accepted");
  check(guarded.coherentPattern(3,readback)&&readback==good,
        "invalid bulk calls changed live pattern");
  // Project saves run on the control thread and must not serialize a
  // hybrid of two concurrent full-pattern replacements (T05).
  {
    AtomicPatternBank exportBank;
    std::array<std::uint8_t,kRows*kSteps> a{},b{};
    a.fill(21);b.fill(105);
    check(exportBank.replacePattern(0,a.data(),a.size()),"T05 initial pattern");
    std::binary_semaphore begin{0};
    std::atomic<bool> finished{false},torn{false};
    std::thread editing([&] {
      begin.acquire();
      for(unsigned iteration=0;iteration<1800;++iteration){
        const auto& candidate=(iteration&1)?a:b;
        if(!exportBank.replacePattern(0,candidate.data(),candidate.size()))
          std::terminate();
      }
      finished.store(true,std::memory_order_release);
    });
    std::size_t reads=0;
    begin.release();
    do {
      const auto snapshot=exportBank.snapshotCoherentForState();
      const auto first=snapshot[0];
      if(first!=21&&first!=105)torn.store(true);
      for(std::size_t i=1;i<kRows*kSteps;++i)
        if(snapshot[i]!=first)torn.store(true);
      ++reads;
    }while(!finished.load(std::memory_order_acquire));
    editing.join();
    check(reads>0&&!torn.load(),"T05 torn full-pattern project snapshot");
  }
  // Kit recall publishes all eight pattern slices as complete generations.
  AtomicPatternBank recalled;
  std::array<std::uint8_t,kCells> recalledGrid{};
  for(std::size_t pattern=0;pattern<kPatterns;++pattern)
    for(std::size_t i=0;i<kRows*kSteps;++i)
      recalledGrid[pattern*kRows*kSteps+i]=static_cast<std::uint8_t>(pattern*13);
  for(std::size_t pattern=0;pattern<kPatterns;++pattern)
    check(recalled.replacePattern(pattern,
             recalledGrid.data()+pattern*kRows*kSteps,kRows*kSteps),
          "kit pattern generation publication failed");
  std::array<std::uint8_t,kRows*kSteps> recalledPattern{};
  for(std::size_t pattern=0;pattern<kPatterns;++pattern){
    check(recalled.coherentPattern(pattern,recalledPattern),
          "kit pattern snapshot unavailable");
    for(auto v:recalledPattern)
      check(v==pattern*13,"kit recalled incomplete pattern");
  }
  // Audit B01: concurrent full-pattern replacement must never expose
  // a hybrid of two generations to the scheduler's coherent reader.
  AtomicPatternBank transactionBank;
  std::array<std::uint8_t,kRows*kSteps> oldPattern{},newPattern{},coherent{};
  oldPattern.fill(15);newPattern.fill(115);
  check(transactionBank.replacePattern(0,oldPattern.data(),oldPattern.size()),"initial transaction");
  std::atomic<bool> replaceDone{false};
  std::binary_semaphore readerReady{0},firstRead{0};
  std::thread replacer([&]{
    readerReady.acquire();
    firstRead.acquire();
    for(int iteration=0;iteration<2000;++iteration){
      const auto& source=(iteration&1)?oldPattern:newPattern;
      if(!transactionBank.replacePattern(0,source.data(),source.size()))
        std::terminate();
    }
    replaceDone.store(true,std::memory_order_release);
  });
  std::size_t verified=0;
  readerReady.release();
  check(transactionBank.coherentPattern(0,coherent),"T03 first reader snapshot");
  ++verified;firstRead.release();
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
      const auto expectedVelocity=coherent[0];
      const auto scheduled=StepScheduler::run(stable,Clock{0,120,48000,256,true},
          [&](Trigger hit) {
            check(hit.velocity==expectedVelocity,
                  "scheduler emitted mixed pattern generations");
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
  // V1A02: the musical duration and all cells form one versioned snapshot.
  // A reader may observe the old or the new generation, never a mixed pair.
  {
    AtomicPatternBank durationBank;
    std::array<std::uint8_t,kRows*kSteps> first{},second{},read{};
    first.fill(0);second.fill(0);first[0]=37;second[0]=100;
    check(durationBank.replacePattern(0,first.data(),first.size(),32),
          "V1A02 initial 2T version");
    std::atomic<bool> done{false};
    std::thread durationWriter([&]{
      for(int i=0;i<10000;++i){
        if(!durationBank.replacePattern(0,(i&1)?first.data():second.data(),
                first.size(),(i&1)?32:16))std::terminate();
      }
      done.store(true,std::memory_order_release);
    });
    std::size_t accepted=0;
    while(!done.load(std::memory_order_acquire)){
      std::uint8_t duration=0;
      if(durationBank.coherentPattern(0,read,duration)){
        check((duration==32&&read==first)||(duration==16&&read==second),
              "V1A02 length/steps mixed across versions");
        ++accepted;
      }
    }
    durationWriter.join();
    std::uint8_t finalDuration=0;
    check(durationBank.coherentPattern(0,read,finalDuration),
          "V1A02 final duration snapshot");
    check((finalDuration==32&&read==first)||(finalDuration==16&&read==second),
          "V1A02 final mixed state");
    check(!durationBank.setLength(0,17),"V1A02 invalid duration accepted");
    check(durationBank.setLength(0,16),"V1A02 length-only update rejected");
    check(durationBank.coherentPattern(0,read,finalDuration)&&finalDuration==16,
          "V1A02 length-only snapshot failed");
    check(accepted>0,"V1A02 no successful concurrent snapshots");
  }
  check(verified>0,"T03 no successful snapshots");
  // Exact mid-write/read overlap is additionally forced in the Windows
  // integration target using test-only generated-source probe points.
  std::cout<<"Atomic pattern bank: PASS\n";
  return 0;
 }catch(const std::exception& ex){
  std::cerr<<"Atomic pattern bank: FAIL "<<ex.what()<<'\n';
  return 1;
 }
}
