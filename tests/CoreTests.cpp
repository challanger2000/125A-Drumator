#include "core/SequencerCore.h"
#include "core/MasterGainRamp.h"
#include "core/RowFill.h"
#include "core/PatternOperations.h"
#include "core/SongChain.h"
#include "core/SampleFolderNavigation.h"
#include "core/CompactRowViewport.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <vector>
using namespace drumator;
static void check(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
static_assert(filledRow(RowFill::Offbeat8)[1]==127&&filledRow(RowFill::Offbeat8)[0]==0);
static_assert(filledRow(RowFill::Gallop)[2]==95&&filledRow(RowFill::Gallop)[3]==84);
static_assert(filledRow(RowFill::SnareGhost)[4]==127&&filledRow(RowFill::SnareGhost)[11]==42);
static_assert(filledRow(RowFill::Clear)[0]==0);

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
  // G01: 5ms master fade is monotonic, bounded and block-invariant.
  for(const double rate:{44100.,48000.,96000.}) {
    MasterGainRamp whole,split;
    check(whole.prepare(rate)&&split.prepare(rate),"G01 valid rate");
    whole.reset(1.f);split.reset(1.f);
    whole.request(2.f);split.request(2.f);
    const int length=static_cast<int>(std::ceil(rate*MasterGainRamp::kTransitionSeconds));
    std::vector<float> reference,partitioned;
    for(int i=0;i<length+256;++i)reference.push_back(whole.next());
    for(int i=0;i<length+256;) {
      const int frames=std::min(127,length+256-i);
      for(int j=0;j<frames;++j)partitioned.push_back(split.next());
      i+=frames;
    }
    check(reference.size()==partitioned.size(),"G01 frame count");
    for(std::size_t i=0;i<reference.size();++i) {
      check(std::abs(reference[i]-partitioned[i])<1.e-7f,
            "G01 master depends on host block partition");
      if(i>0)check(reference[i]>=reference[i-1]&&
                   reference[i]-reference[i-1]<=1.1f/length,
                   "G01 discontinuous/overshooting gain");
    }
    check(reference.back()==2.f,"G01 maximum");
    whole.request(0.f);
    check(whole.next()>1.9f,"G01 hard jump after parameter change");
    for(int i=1;i<length;++i)whole.next();
    check(whole.next()==0.f,"G01 zero endpoint");
    whole.request(2.f);
    for(int i=0;i<length/2;++i)whole.next();
    const float midpoint=whole.next();
    whole.request(1.f);
    check(std::abs(whole.next()-midpoint)<2.f/length,
          "G01 mid-ramp automation created click");
  }
  { MasterGainRamp invalid;
    check(!invalid.prepare(0.)&&
          !invalid.prepare(std::numeric_limits<double>::quiet_NaN()),
          "G01 invalid sample rate accepted");
  }
  // B04: every instrument remains reachable when the host rejects resize.
  for(std::size_t total=1;total<=16;++total) {
    std::array<bool,16> visited{};
    std::size_t first=0;
    for(std::size_t page=0;page<16;++page) {
      const auto n=CompactRowViewport::count(total,first);
      check(n>=1&&n<=4,"invalid fallback page size");
      for(std::size_t i=0;i<n;++i) {
        const auto row=CompactRowViewport::row(total,first,i);
        check(row<total,"fallback hit maps outside instrument count");
        visited[row]=true;
      }
      const auto next=CompactRowViewport::next(total,first);
      if(next==first)break;
      check(next>first,"fallback pagination must advance");
      first=next;
    }
    for(std::size_t row=0;row<total;++row)
      check(visited[row],"inaccessible instrument after rejected resize");
    while(first>0)first=CompactRowViewport::previous(total,first);
    check(first==0,"fallback cannot return to first page");
  }
  check(CompactRowViewport::count(0,0)==0&&
        CompactRowViewport::next(0,0)==0&&
        CompactRowViewport::row(0,0,0)==0,
        "fallback empty-state bounds");
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
  // Vary song entry lengths and verify exact placement through the scheduler.
  {
    SongChain chain;
    check(chain.setCount(3),"chain count");
    check(chain.configure(0,0,16)&&chain.configure(1,1,8)&&chain.configure(2,2,32),
          "chain lengths");
    struct SongBank {
      SongChain& chain;
      std::uint8_t selected() const noexcept {return 0;}
      SongChain::Position mapStep(std::int64_t step,std::int64_t origin) const noexcept {
        return chain.at(step,origin);
      }
      std::uint8_t get(std::size_t p,std::size_t row,std::size_t step) const noexcept {
        return row==0&&step==0&&p<=2?static_cast<std::uint8_t>(p+90):0;
      }
    } bank{chain};
    std::vector<Trigger> hits;
    const auto result=StepScheduler::run(bank,Clock{0,120.,48000.,336000,true},
        [&](Trigger hit){hits.push_back(hit);});
    check(result.status==ScheduleStatus::Ok&&hits.size()==3,"song event count");
    check(hits[0].pattern==0&&hits[0].step==0&&hits[0].sampleOffset==0,
          "song A first bar");
    check(hits[1].pattern==1&&hits[1].step==0&&hits[1].sampleOffset==96000,
          "song B half-bar fill");
    check(hits[2].pattern==2&&hits[2].step==0&&hits[2].sampleOffset==144000,
          "song C follows fill");
  }
  // A-B-A at bar 2: 3 one-bar entries, invariant to buffer subdivision.
  {
    SongChain chain;
    check(chain.setCount(3)&&chain.configure(0,0,16)&&
          chain.configure(1,1,16)&&chain.configure(2,0,16),"ABA chain");
    struct Bank {
      SongChain& chain;
      std::uint8_t selected() const noexcept {return 0;}
      SongChain::Position mapStep(std::int64_t x,std::int64_t origin) const noexcept {
        return chain.at(x,origin);
      }
      std::uint8_t get(std::size_t p,std::size_t r,std::size_t step) const noexcept {
        return r==p&&step==0?127:0;
      }
    } bank{chain};
    std::vector<Trigger> whole,split;
    const auto runABA=[&](std::uint32_t start,std::uint32_t frames,std::vector<Trigger>& out){
      const auto ppq=4.0+static_cast<double>(start)*120.0/(60.0*48000.0);
      const auto result=StepScheduler::run(bank,Clock{ppq,120.,48000.,
        static_cast<int>(frames),true,0.,0.,16},[&](Trigger t){
        t.sampleOffset+=start;out.push_back(t);
      });
      check(result.status==ScheduleStatus::Ok,"ABA scheduler status");
    };
    for(std::uint32_t i=0;i<288000;i+=96000)runABA(i,96000,whole);
    for(std::uint32_t i=0;i<288000;){
      const auto n=std::min(std::uint32_t{511},288000-i);
      runABA(i,n,split);i+=n;
    }
    check(whole.size()==3&&split.size()==3,"ABA event count");
    for(std::size_t i=0;i<3;++i){
      const auto expected=static_cast<std::uint8_t>(i==1?1:0);
      const auto offset=static_cast<std::uint32_t>(i*96000);
      check(whole[i].pattern==expected&&whole[i].row==expected&&
            whole[i].step==0&&whole[i].sampleOffset==offset,
            "ABA wrong pattern or downbeat");
      check(split[i].pattern==expected&&split[i].sampleOffset==offset,
            "ABA buffer dependence");
    }
  }
  // Per-instrument FILL must change exactly one row, and never touch the
  // other instruments, even for ghost/galopp and clearing operations.
  {
    PatternSteps initial{};
    for(std::size_t row=0;row<kRows;++row)
      for(std::size_t step=0;step<kSteps;++step)
        initial[row*kSteps+step]=static_cast<std::uint8_t>(
          ((row*7+step*3)%5==0)?96:0);
    const std::array<RowFill,9> all{
      RowFill::Every2,RowFill::Every4,RowFill::Every8,
      RowFill::Snare24,RowFill::All,RowFill::Clear,
      RowFill::Offbeat8,RowFill::Gallop,RowFill::SnareGhost
    };
    for(const auto mode:all){
      const auto changed=fillInstrument(initial,7,mode);
      const auto expected=filledRow(mode);
      for(std::size_t row=0;row<kRows;++row)
        for(std::size_t step=0;step<kSteps;++step)
          check(changed[row*kSteps+step]==
                    (row==7?expected[step]:initial[row*kSteps+step]),
                "FILL modified another instrument or wrong step/velocity");
    }
    check(fillInstrument(initial,kRows,RowFill::All)==initial,
          "Invalid FILL row must not change pattern");
    check(filledRow(RowFill::Snare24)[4]==127&&
          filledRow(RowFill::Snare24)[12]==127&&
          filledRow(RowFill::Snare24)[20]==127&&
          filledRow(RowFill::Snare24)[28]==127,
          "Snare 2/4 incorrect in the second bar");
  }
  std::cout<<"Drumator step core: all test groups PASS\n";
  return 0;
 } catch(const std::exception& ex){
  std::cerr<<"Drumator step core FAIL: "<<ex.what()<<'\n';
  return 1;
 }
}
