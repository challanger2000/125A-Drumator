#include "core/AtomicPatternBank.h"
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
  AtomicPatternBank bank;
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
  std::cout<<"Atomic pattern bank: PASS\n";
  return 0;
 }catch(const std::exception& ex){
  std::cerr<<"Atomic pattern bank: FAIL "<<ex.what()<<'\n';
  return 1;
 }
}
