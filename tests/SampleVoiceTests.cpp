#include "core/SampleVoiceEngine.h"
#include "core/RowMuteSolo.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>

using namespace drumator;
static void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
static void near(double x,double expected,double tolerance,const char* msg){
 if(!std::isfinite(x)||std::abs(x-expected)>tolerance)
    throw std::runtime_error(msg);
}
static void testMuteSoloPolicy() {
 RowMuteSolo gate;
 require(gate.audible(0)&&gate.audible(15),"empty solos must allow all");
 gate.mute[1]=true;
 require(!gate.audible(1)&&gate.audible(2),"mute must affect only its row");
 gate.solo[2]=true;
 require(gate.audible(2)&&!gate.audible(0),"solo must isolate selected rows");
 gate.solo[3]=true;
 require(gate.audible(3),"multiple solo rows must remain audible");
 gate.mute[3]=true;
 require(!gate.audible(3),"mute must override solo");
 require(!gate.audible(16),"out-of-range row must be silent");
 gate.solo.fill(false);
 require(gate.audible(0)&&!gate.audible(1),"releasing solo must restore nonmuted rows");
}
static void testKernel() {
 SincKernel kernel;
 require(!kernel.prepare(0.0)&&!kernel.prepare(-1.0),"invalid ratio accepted");
 require(kernel.prepare(1.0)&&kernel.ready(),"kernel 1x failed");
 std::array<float,512> constant{};
 constant.fill(1.f);
 SampleVoiceEngine engine;
 require(engine.setOutputRate(48000),"output rate rejected");
 require(!engine.setOutputRate(0.0),"bad output rate accepted");
 RowSound row;
 row.sample={constant.data(),nullptr,constant.size()};
 row.kernel=&kernel;row.pan=-1.f;
 require(engine.setRow(0,row),"row configuration rejected");
 require(!engine.setRow(16,row),"invalid row accepted");
 row.gain=-1.f;
 require(!engine.setRow(0,row),"invalid gain accepted");
 std::array<float,256> l{},r{};
 const Trigger t{0,0,0,127,0};
 const auto v=engine.render(&t,1,l.data(),r.data(),256);
 require(v.status==RenderStatus::Ok&&v.accepted==1,"basic render failed");
 for(std::size_t i=32;i<200;++i) {
    near(l[i],1.0,0.0005,"DC interpolation not unity gain");
    near(r[i],0.0,0.0001,"hard left pan leaked right");
 }
}
static void testRowMix() {
 std::array<float,1024> source{};source.fill(1.f);
 SincKernel kernel;require(kernel.prepare(1),"mix kernel");
 SampleVoiceEngine engine;require(engine.setOutputRate(48000),"mix rate");
 RowSound row;row.sample={source.data(),nullptr,source.size()};
 row.kernel=&kernel;
 require(engine.setRow(0,row),"mix row");
 require(!engine.setRowMix(16,1.f,0.f)&&
         !engine.setRowMix(0,-1.f,0.f)&&
         !engine.setRowMix(0,1.f,2.f),"invalid mix accepted");
 require(engine.setRowMix(0,.5f,-1.f),"gain pan update");
 std::array<float,256> l{},r{};
 const Trigger hit{0,0,0,127,0};
 require(engine.render(&hit,1,l.data(),r.data(),256).accepted==1,
         "mix event");
 near(l[120],.5,.001,"row gain not applied");
 near(r[120],0,.001,"row left pan not applied");
 engine.reset();
 require(engine.setRowMix(0,1.f,1.f),"right pan update");
 require(engine.render(&hit,1,l.data(),r.data(),256).accepted==1,
         "right pan event");
 near(l[120],0,.001,"right pan leaked left");
 near(r[120],1,.001,"right pan amplitude");
}
static void testRetriggerAndMute() {
 std::array<float,512> positive{};
 positive.fill(1.f);
 SincKernel kernel;require(kernel.prepare(1),"kernel");
 SampleVoiceEngine engine;
 require(engine.setOutputRate(44100),"rate");
 RowSound row;row.sample={positive.data(),nullptr,positive.size()};
 row.kernel=&kernel;row.pan=-1.f;
 require(engine.setRow(0,row),"set row");
 const std::array<Trigger,2> t{{{0,0,0,127,0},{16,0,1,127,0}}};
 std::array<float,64> l{},r{};
 auto result=engine.render(t.data(),t.size(),l.data(),r.data(),64);
 require(result.accepted==2&&result.activeVoices==2,"retrigger did not overlap");
 near(l[10],1,0.001,"first voice incorrect");
 near(l[20],2,0.001,"overlapping one-shots not mixed");
 row.mute=true;
 require(engine.setRow(0,row),"mute");
 result=engine.render(&t[0],1,l.data(),r.data(),64);
 require(result.accepted==0&&result.activeVoices==2,"mute incorrectly killed tails");
 near(l[20],2,0.001,"mute unexpectedly stopped ongoing voice");
}
static void testSampleSwapLifetime() {
 std::array<float,512> original{},replacement{};
 original.fill(1.f);replacement.fill(-1.f);
 SincKernel kernel;require(kernel.prepare(1),"kernel");
 SampleVoiceEngine engine;require(engine.setOutputRate(48000),"rate");
 RowSound row;row.pan=-1.f;row.kernel=&kernel;
 row.sample={original.data(),nullptr,original.size()};
 require(engine.setRow(0,row),"install original");
 std::array<float,64> l{},r{};
 const Trigger a{0,0,0,127,0};
 require(engine.render(&a,1,l.data(),r.data(),64).accepted==1,"first trigger");
 row.sample={replacement.data(),nullptr,replacement.size()};
 require(engine.setRow(0,row),"replace sample");
 require(engine.render(&a,1,l.data(),r.data(),64).accepted==1,"second trigger");
 near(l[20],0,0.002,"old voice did not retain original sample");
 // Only legal while both backing arrays stay alive; engine cannot own them.
}
static void testChokeAndStereo() {
 std::array<float,1024> open{},silent{},leftInput{},rightInput{};
 open.fill(1.f);leftInput.fill(.5f);rightInput.fill(-.25f);
 SincKernel kernel;require(kernel.prepare(1),"kernel");
 SampleVoiceEngine engine;require(engine.setOutputRate(48000,2),"rate");
 RowSound row;row.kernel=&kernel;row.pan=-1.f;row.chokeGroup=1;
 row.sample={open.data(),nullptr,open.size()};
 require(engine.setRow(0,row),"open");
 row.sample={silent.data(),nullptr,silent.size()};
 require(engine.setRow(1,row),"closed");
 std::array<float,256> l{},r{};
 const Trigger start{0,0,0,127,0},choke{0,1,0,127,0};
 require(engine.render(&start,1,l.data(),r.data(),128).accepted==1,"open trigger");
 require(engine.render(&choke,1,l.data(),r.data(),256).accepted==1,"closed trigger");
 near(l[0],1,0.001,"choke incorrectly killed sample immediately");
 near(l[110],0,0.0001,"choke fade did not finish");
 require(engine.active()==1,"choked voice retained");
 engine.reset();
 row.chokeGroup=0;row.pan=-1.f;
 row.sample={leftInput.data(),rightInput.data(),leftInput.size()};
 require(engine.setRow(2,row),"stereo");
 const Trigger stereo{0,2,0,127,0};
 require(engine.render(&stereo,1,l.data(),r.data(),256).accepted==1,"stereo start");
 near(l[50],.5,0.0005,"stereo left altered");
 near(r[50],0,0.0001,"hard left not panned");
 row.pan=0;
 require(engine.setRow(2,row),"center row");
 engine.reset();
 require(engine.render(&stereo,1,l.data(),r.data(),256).accepted==1,"stereo center");
 near(l[50],.5,.001,"center L WAV unity amplitude");
 near(r[50],-.25,.001,"center R WAV unity amplitude");
}
static void testCapacityValidationAndAutomation() {
 std::array<float,1000> source{};
 source.fill(1.f);
 SincKernel kernel;require(kernel.prepare(1),"kernel");
 SampleVoiceEngine engine;require(engine.setOutputRate(48000),"rate");
 RowSound row;row.sample={source.data(),nullptr,source.size()};
 row.kernel=&kernel;row.pan=-1.f;
 require(engine.setRow(0,row),"row");
 std::array<Trigger,65> events{};
 for(auto& e:events)e={0,0,0,127,0};
 std::array<float,8> l{},r{};
 auto result=engine.render(events.data(),events.size(),l.data(),r.data(),8);
 require(result.status==RenderStatus::Ok&&result.accepted==64&&
         result.rejectedCapacity==1&&engine.active()==64,"voice capacity accounting");
 near(l[0],64.0,0.005,"full-polyphony summed amplitude");
 const auto active=engine.active();
 std::array<Trigger,2> malformed{{{2,0,0,127,0},{1,0,0,127,0}}};
 l.fill(42.f);
 result=engine.render(malformed.data(),malformed.size(),l.data(),r.data(),8);
 require(result.status==RenderStatus::InvalidEvents&&engine.active()==active,
         "malformed events changed voice state");
 near(l[0],42,0,"malformed events modified caller buffer");
 const Trigger bad{0,0,0,0,0};
 result=engine.render(&bad,1,l.data(),r.data(),8);
 require(result.status==RenderStatus::InvalidEvents,"invalid velocity accepted");
 require(engine.active()==active,"invalid velocity modified voices");
}
static void testBandlimiting() {
 constexpr double pi=3.141592653589793238462643383279502884;
 std::vector<float> high(4096),low(4096);
 for(std::size_t i=0;i<high.size();++i) {
   high[i]=static_cast<float>(std::sin(2*pi*.40*i));
   low[i]=static_cast<float>(std::sin(2*pi*.05*i));
 }
 SincKernel kernel;require(kernel.prepare(2),"pitch + one octave");
 SampleVoiceEngine e;require(e.setOutputRate(48000),"rate");
 RowSound row;row.pan=-1.f;row.kernel=&kernel;
 std::vector<float> l(1000),r(1000);
 const Trigger t{0,0,0,127,0};
 row.sample={high.data(),nullptr,high.size()};
 require(e.setRow(0,row),"high band");
 require(e.render(&t,1,l.data(),r.data(),1000).accepted==1,"high render");
 double powerHigh=0;
 for(std::size_t i=150;i<850;++i)powerHigh+=static_cast<double>(l[i])*l[i];
 powerHigh=std::sqrt(powerHigh/700);
 row.sample={low.data(),nullptr,low.size()};
 require(e.setRow(0,row),"low band");
 e.reset();
 require(e.render(&t,1,l.data(),r.data(),1000).accepted==1,"low render");
 double powerLow=0;
 for(std::size_t i=150;i<850;++i)powerLow+=static_cast<double>(l[i])*l[i];
 powerLow=std::sqrt(powerLow/700);
 require(powerLow>.5,"low-band tone incorrectly attenuated");
 require(powerHigh/powerLow<.02,"alias rejection for octave-up insufficient");
}
static void testStressAndRecovery() {
 std::array<float,2048> sample{};sample.fill(.125f);
 SincKernel kernel;require(kernel.prepare(1.),"stress kernel");
 SampleVoiceEngine engine;require(engine.setOutputRate(48000),"stress rate");
 RowSound row;row.sample={sample.data(),nullptr,sample.size()};
 row.kernel=&kernel;row.pan=-1.f;
 for(std::size_t i=0;i<kRows;++i)
   require(engine.setRow(i,row),"stress row");
 std::array<Trigger,128> hits{};
 for(std::size_t i=0;i<hits.size();++i)
   hits[i]={0,static_cast<std::uint8_t>(i%kRows),0,127,0};
 std::array<float,256> l{},r{};
 for(int round=0;round<6;++round) {
   engine.reset();
   auto result=engine.render(hits.data(),hits.size(),l.data(),r.data(),256);
   require(result.status==RenderStatus::Ok&&result.accepted==64&&
           result.rejectedCapacity==64&&result.activeVoices==64,
           "dense triggers exceeded 64-voice budget");
   for(int n=0;n<256;++n)
     require(std::isfinite(l[n])&&std::isfinite(r[n]),
             "nonfinite output under 128 simultaneous MIDI hits");
   for(int tail=0;tail<9;++tail) {
     result=engine.render(nullptr,0,l.data(),r.data(),256);
     require(result.status==RenderStatus::Ok,"stress drain");
   }
   require(engine.active()==0,"exhausted voices not reclaimed");
   const Trigger next{0,0,0,127,0};
   require(engine.render(&next,1,l.data(),r.data(),256).accepted==1,
           "voice pool cannot recover from overload");
 }
}
static void testSimultaneousStereoStems() {
 std::array<float,1024> sample{};sample.fill(.25f);
 SincKernel kernel;require(kernel.prepare(1.),"stems kernel");
 SampleVoiceEngine engine;require(engine.setOutputRate(48000),"stems rate");
 RowSound row;row.sample={sample.data(),nullptr,sample.size()};
 row.kernel=&kernel;
 for(std::size_t i=0;i<3;++i)
   require(engine.setRow(i,row),"stems row");
 std::array<float,256> mainL{},mainR{},stem1L{},stem1R{},stem2L{},stem2R{};
 std::array<float*,kRows> stemLeft{},stemRight{};
 stemLeft[1]=stem1L.data();stemRight[1]=stem1R.data();
 stemLeft[2]=stem2L.data();stemRight[2]=stem2R.data();
 const std::array<Trigger,3> hits{{{0,0,0,127,0},
                                   {0,1,0,127,0},
                                   {0,2,0,127,0}}};
 require(engine.render(hits.data(),hits.size(),mainL.data(),mainR.data(),256,
                       stemLeft.data(),stemRight.data()).accepted==3,
         "simultaneous stems launch");
 near(mainL[120],.25,.001,"main gain isolated");
 near(stem1L[120],.25,.001,"stem one left");
 near(stem2L[120],.25,.001,"stem two left");
 near(mainR[120],.25,.001,"main stereo");
 near(stem1R[120],.25,.001,"stem one stereo");
 near(stem2R[120],.25,.001,"stem two stereo");
 stemLeft[2]=nullptr;stemRight[2]=nullptr;
 require(engine.render(nullptr,0,mainL.data(),mainR.data(),256,
                       stemLeft.data(),stemRight.data()).status==RenderStatus::Ok,
         "stem dropout render");
 near(mainL[120],.5,.001,"disabled stem should sum into main");
 near(stem1L[120],.25,.001,"other stem must remain isolated");
}
int main() {
 try {
  {
    // Regression: an empty instrument must remain silent, even if the
    // sequencer continues emitting the old factory step pattern.
    SampleVoiceEngine empty;
    require(empty.setOutputRate(44100),"empty voice setup");
    std::array<float,128> left{},right{};
    left.fill(1.f);right.fill(1.f);
    const Trigger hit{0,0,0,127,0};
    const auto result=empty.render(&hit,1,left.data(),right.data(),128);
    require(result.accepted==0&&result.activeVoices==0,
            "factory test kick still triggered in empty instrument");
    for(std::size_t i=0;i<left.size();++i)
      require(left[i]==0.f&&right[i]==0.f,"empty sampler produced sound");
  }
  testMuteSoloPolicy();testKernel();testRowMix();testRetriggerAndMute();testSampleSwapLifetime();
  testChokeAndStereo();testCapacityValidationAndAutomation();testBandlimiting();
  testStressAndRecovery();testSimultaneousStereoStems();
  std::cout<<"Drumator sample-voice regression: 10 groups PASS\n";
  return 0;
 } catch(const std::exception& ex) {
  std::cerr<<"Drumator sample-voice regression FAIL: "<<ex.what()<<'\n';
  return 1;
 }
}
