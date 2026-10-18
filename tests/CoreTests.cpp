#include "core/SequencerCore.h"
#include "core/SampleFolderNavigation.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace drumator;
static void check(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
static std::vector<Trigger> get(const PatternBank& b,Clock t,
   ScheduleStatus expected=ScheduleStatus::Ok){
 std::vector<Trigger> a;
 const auto s=StepScheduler::run(b,t,[&](const Trigger& e){a.push_back(e);});
 check(s.status==expected,"scheduler return status");
 check(s.count==a.size(),"scheduler trigger counter");
 return a;
}
int main(){
 try {
  // Sample navigation: sorted, cyclic, unchanged on invalid requests.
  const std::vector<std::string> files{"D.wav","A.wav","B.flac"};
  std::string selected="keep";
  check(adjacentSample(files,"A.wav",1,selected)&&selected=="B.flac",
        "folder next");
  check(adjacentSample(files,"A.wav",-1,selected)&&selected=="D.wav",
        "folder backward wrap");
  check(adjacentSample(files,"D.wav",1,selected)&&selected=="A.wav",
        "folder forward wrap");
  check(!adjacentSample(files,"missing.wav",1,selected)&&selected=="A.wav",
        "missing current sample changed selection");
  check(!adjacentSample(files,"A.wav",0,selected),"invalid navigation direction");
  check(!adjacentSample({"same.wav"},"same.wav",1,selected),
        "single sample must not reload");
  check(adjacentSample({"B.wav","A.wav","B.wav"},"A.wav",1,selected)&&
        selected=="B.wav","duplicate filename handling");
  PatternBank b;
  check(b.get(0,0,0)==0,"initial silent grid");
  check(b.set(0,0,0,127)&&b.set(0,1,1,100)&&b.set(0,2,2,66)&&
        b.set(0,15,31,1),"valid step positions");
  check(!b.set(8,0,0,1)&&!b.set(0,16,0,1)&&!b.set(0,0,32,1)&&
        !b.set(0,0,0,128),"out of range validation");
  auto e=get(b,Clock{0,120,48000,12001,true});
  check(e.size()==3&&e[0].sampleOffset==0&&e[1].sampleOffset==6000&&
        e[2].sampleOffset==12000,"step sample positions");
  e=get(b,Clock{0,120,48000,6000,true});
  check(e.size()==1&&e[0].row==0,"end boundary should be exclusive");
  e=get(b,Clock{7.75,120,48000,12000,true});
  check(e.size()==2&&e[0].step==31&&e[1].step==0&&
        e[1].sampleOffset==6000,"32-step wrap");
  // Starting playback at DAW bar 2 (PPQ 4) must begin at step 1,
  // not at the previous absolute 16th index 17.
  e=get(b,Clock{4,120,48000,12001,true,0,0,16});
  check(e.size()==3&&e[0].step==0&&e[0].sampleOffset==0&&
        e[1].step==1&&e[2].step==2,"bar 2 start origin");
  e=get(b,Clock{4.25,120,48000,6001,true,0,0,16});
  check(e.size()==2&&e[0].step==1&&e[1].step==2,
        "transport origin lost on continuous block");
  e=get(b,Clock{-0.25,120,48000,12000,true});
  check(e.size()==2&&e[0].step==31&&e[1].step==0,"negative PPQ");
  check(get(b,Clock{0.10,120,48000,3600,true}).empty(),"midstep onset");
  e=get(b,Clock{0.10,120,48000,3601,true});
  check(e.size()==1&&e[0].sampleOffset==3600,"seek and mid-block onset");
  check(get(b,Clock{0,120,48000,256,false}).empty(),"stopped transport");
  for(std::size_t i=0;i<kRows;++i)
      check(b.set(7,i,0,static_cast<unsigned>(i+1)),"all rows");
  check(b.select(7)&&!b.select(8),"pattern selection range");
  e=get(b,Clock{0,120,44100,256,true});
  check(e.size()==16,"16 simultaneous drum rows");
  for(std::size_t i=0;i<e.size();++i)
      check(e[i].row==i&&e[i].pattern==7&&e[i].velocity==i+1,
            "velocity and row/pattern identity");
  // Swing delays odd 16th boundaries by 25% without moving even steps.
  PatternBank swung;
  check(swung.set(0,0,0,127)&&swung.set(0,0,1,127)&&
        swung.set(0,0,2,127),"swing fixture");
  auto delayed=get(swung,Clock{0,120,48000,15001,true,0.25});
  check(delayed.size()==3&&delayed[0].sampleOffset==0&&
        delayed[1].sampleOffset==7500&&delayed[2].sampleOffset==12000,
        "swing positions incorrect");
  delayed=get(swung,Clock{0.25,120,48000,6501,true,0.25});
  check(delayed.size()==2&&delayed[0].step==1&&
        delayed[0].sampleOffset==1500&&delayed[1].step==2&&
        delayed[1].sampleOffset==6000,
        "swing boundary lost in split host block");
  check(get(swung,Clock{0,120,48000,100,true,0.75},
        ScheduleStatus::InvalidClock).empty(),"invalid swing accepted");
  // Humanize must repeat exactly and survive host buffer partitioning.
  auto human=get(swung,Clock{0,120,48000,24000,true,0.0,1.0});
  auto repeat=get(swung,Clock{0,120,48000,24000,true,0.0,1.0});
  check(human.size()==repeat.size(),"humanize changed event count");
  for(std::size_t i=0;i<human.size();++i)
    check(human[i].sampleOffset==repeat[i].sampleOffset&&
          human[i].velocity==repeat[i].velocity,"humanize not deterministic");
  std::vector<Trigger> splitHuman;
  for(int start=0;start<24000;start+=1000) {
    const auto ppq=static_cast<double>(start)*120/(60*48000);
    for(auto hit:get(swung,Clock{ppq,120,48000,1000,true,0.0,1.0})) {
      hit.sampleOffset+=static_cast<std::uint32_t>(start);
      splitHuman.push_back(hit);
    }
  }
  check(human.size()==splitHuman.size(),"humanize split event count");
  for(std::size_t i=0;i<human.size();++i)
    check(human[i].sampleOffset==splitHuman[i].sampleOffset&&
          human[i].velocity==splitHuman[i].velocity,
          "humanize changed with block size");
  check(get(swung,Clock{0,120,48000,128,true,0,1.1},
       ScheduleStatus::InvalidClock).empty(),"invalid humanize accepted");
  const auto snapshot=b.save();
  PatternBank restored;
  check(restored.load(snapshot.data(),snapshot.size()),"valid restore");
  check(restored.save()==snapshot,"state roundtrip");
  auto damaged=snapshot;damaged[4]=2;
  check(!restored.load(damaged.data(),damaged.size()),"unsupported version");
  damaged=snapshot;damaged.back()=255;
  check(!restored.load(damaged.data(),damaged.size()),"late invalid velocity");
  damaged=snapshot;damaged[5]=8;
  check(!restored.load(damaged.data(),damaged.size()),"invalid active pattern");
  check(!restored.load(snapshot.data(),snapshot.size()-1),"truncated state");
  check(!restored.load(snapshot.data(),snapshot.size()+1),"oversized state");
  check(!restored.load(nullptr,snapshot.size()),"null state");
  check(restored.save()==snapshot,"failed parse mutated previous state");
  check(get(b,Clock{0,0,48000,256,true},ScheduleStatus::InvalidClock).empty(),
        "invalid BPM handling");
  check(get(b,Clock{0,120,0,256,true},ScheduleStatus::InvalidClock).empty(),
        "invalid sample rate");
  check(get(b,Clock{0,120,48000,-1,true},ScheduleStatus::InvalidClock).empty(),
        "invalid buffer length");
  check(get(b,Clock{0,1000000,48000,10000,true},
            ScheduleStatus::TooManyBoundaries).empty(),
        "work cap must not partially emit");
  PatternBank dense;
  for(std::size_t i=0;i<32;++i)check(dense.set(0,0,i,64),"dense grid");
  constexpr std::uint32_t total=192001;
  std::vector<std::uint32_t> ref,partition;
  for(auto x:get(dense,Clock{0,120,48000,static_cast<int>(total),true}))
      ref.push_back(x.sampleOffset);
  std::array<std::uint32_t,7> chunks{{1,17,255,512,1024,37,3000}};
  std::uint32_t cursor=0;std::size_t k=0;
  while(cursor<total){
   const auto n=std::min(chunks[k++%chunks.size()],total-cursor);
   const double ppq=static_cast<double>(cursor)*120/(60*48000);
   for(auto x:get(dense,Clock{ppq,120,48000,static_cast<int>(n),true}))
       partition.push_back(cursor+x.sampleOffset);
   cursor+=n;
  }
  check(ref==partition,"block size affects musical timing");
  check(ref.size()==33&&ref.front()==0&&ref.back()==192000,
        "reference oracle positions");

  // Block-partition invariance at noninteger samples/step across rates.
  for(const auto setting:std::array<std::array<double,2>,3>{{
      {{44100.0,85.5}},{{96000.0,137.5}},{{192000.0,250.0}}}}){
    const auto rate=setting[0],bpm=setting[1];
    constexpr std::uint32_t length=160000;
    std::vector<std::uint32_t> unsplit,split;
    for(auto x:get(dense,Clock{0,bpm,rate,static_cast<int>(length),true}))
        unsplit.push_back(x.sampleOffset);
    std::uint32_t position=0;std::size_t chunk=0;
    while(position<length){
      const auto count=std::min(chunks[chunk++%chunks.size()],length-position);
      const auto ppq=static_cast<double>(position)*bpm/(60.0*rate);
      for(auto x:get(dense,Clock{ppq,bpm,rate,static_cast<int>(count),true}))
          split.push_back(position+x.sampleOffset);
      position+=count;
    }
    check(unsplit==split,"partition mismatch at alternate rate/tempo");
  }
  // Combined swing/humanize must not depend on host buffer partitioning.
  for(double rate : {44100.,48000.}) {
    const double tempo=123.;
    constexpr std::uint32_t totalFrames=96000;
    const auto reference=get(dense,Clock{0,tempo,rate,96000,true,.5,1.});
    std::vector<Trigger> split;
    std::uint32_t swingFrameCursor=0;
    while(swingFrameCursor<totalFrames) {
      const auto count=std::min(std::uint32_t{127},totalFrames-swingFrameCursor);
      const auto ppq=static_cast<double>(swingFrameCursor)*tempo/(60*rate);
      for(auto hit:get(dense,Clock{ppq,tempo,rate,static_cast<int>(count),true,.5,1.})) {
        hit.sampleOffset+=swingFrameCursor;
        split.push_back(hit);
      }
      swingFrameCursor+=count;
    }
    check(reference.size()==split.size(),"swing/humanize partition event count");
    for(std::size_t i=0;i<reference.size();++i)
      check(reference[i].sampleOffset==split[i].sampleOffset&&
            reference[i].velocity==split[i].velocity,
            "swing/humanize changed under split blocks");
  }
  std::cout<<"Drumator step core: all test groups PASS\n";
  return 0;
 } catch(const std::exception& ex){
  std::cerr<<"Drumator step core FAIL: "<<ex.what()<<'\n';
  return 1;
 }
}
