// Actual packaged Windows VST3 instantiation + MIDI and PCM audio smoke.
// Deliberately no Studio One proprietary code. Fails if the plugin compiles
// but doesn't render a real decoded sample on its real output bus.
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "vst3/PluginIDs.h"
#include "core/Utf8Path.h"
#include "integration/ByteStream.h"
#include "integration/ResultLog.h"
#include "core/KitState.h"
#include "core/AtomicPatternBank.h"
#include "core/FlacImport.h"
#include "core/PatternQuantization.h"
#include "public.sdk/source/vst/utility/memoryibstream.h"
#include <windows.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>
#include <limits>

using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {
void require(bool value,const char* message) {
 if(!value) {drumator::qa::result(std::string("Packaged host FAIL: ")+message);std::cerr<<"Packaged Drumator host FAIL: "<<message<<'\n';std::exit(2);}
}
void u16(std::vector<std::uint8_t>& v,std::uint16_t x) {
 v.push_back(static_cast<std::uint8_t>(x));
 v.push_back(static_cast<std::uint8_t>(x>>8));
}
void u32(std::vector<std::uint8_t>& v,std::uint32_t x) {
 for(int b=0;b<4;++b)v.push_back(static_cast<std::uint8_t>(x>>(8*b)));
}
void id(std::vector<std::uint8_t>& v,const char* s) {
 for(int b=0;b<4;++b)v.push_back(static_cast<std::uint8_t>(s[b]));
}
std::vector<std::uint8_t> makeWav(int bits,bool float32,float amplitude) {
 constexpr std::uint32_t frames=1024,rate=48000;
 const auto bps=static_cast<std::uint32_t>(bits/8);
 std::vector<std::uint8_t> w;
 id(w,"RIFF");u32(w,36+frames*bps);id(w,"WAVE");
 id(w,"fmt ");u32(w,16);u16(w,float32?3:1);u16(w,1);
 u32(w,rate);u32(w,rate*bps);u16(w,static_cast<std::uint16_t>(bps));
 u16(w,static_cast<std::uint16_t>(bits));
 id(w,"data");u32(w,frames*bps);
 for(std::uint32_t n=0;n<frames;++n) {
   if(float32) {
     std::uint32_t raw=0;
     static_assert(sizeof(raw)==sizeof(amplitude),"float is not 32 bit");
     std::memcpy(&raw,&amplitude,sizeof(raw));u32(w,raw);
   }else if(bits==16) {
     const auto sample=static_cast<std::int32_t>(std::lround(amplitude*32768.f));
     u16(w,static_cast<std::uint16_t>(sample));
   }else {
     const auto sample=static_cast<std::int32_t>(std::lround(amplitude*8388608.f));
     for(int b=0;b<3;++b)
       w.push_back(static_cast<std::uint8_t>(sample>>(8*b)));
   }
 }
 return w;
}
// Two independent channel codes; mono fixtures cannot detect L/R swaps.
std::vector<std::uint8_t> makeStereoWav(float l,float r) {
 constexpr std::uint32_t frames=1024,rate=48000;
 std::vector<std::uint8_t> w;
 id(w,"RIFF");u32(w,36+frames*8);id(w,"WAVE");
 id(w,"fmt ");u32(w,16);u16(w,3);u16(w,2);
 u32(w,rate);u32(w,rate*8);u16(w,8);u16(w,32);
 id(w,"data");u32(w,frames*8);
 for(std::uint32_t n=0;n<frames;++n)for(float v:{l,r}) {
   std::uint32_t raw=0;std::memcpy(&raw,&v,4);u32(w,raw);
 }
 return w;
}

#include "PackagedFunctionalAudit.inl"
}
int main(int argc,char** argv) {
 require(argc==2,"expected packaged .vst3 binary argument");
 auto* dll=LoadLibraryW(drumator::pathFromUtf8(argv[1]).c_str());
 require(dll!=nullptr,"LoadLibraryW failed");
 using PluginInit=bool (*)();
 auto* init=reinterpret_cast<PluginInit>(GetProcAddress(dll,"InitDll"));
 require(init&&init(),"VST3 module init failed");
 using FactoryFn=IPluginFactory* (*)();
 auto* getFactory=reinterpret_cast<FactoryFn>(GetProcAddress(dll,"GetPluginFactory"));
 require(getFactory!=nullptr,"Missing plugin factory");
 auto* factory=getFactory();require(factory!=nullptr,"Factory null");
 IComponent* component=nullptr;
 require(factory->createInstance(drumator::kProcessorCID.toTUID(),IComponent::iid,
             reinterpret_cast<void**>(&component))==kResultOk&&component,
         "Processor factory instantiation");
 IAudioProcessor* audio=nullptr;
 IConnectionPoint* processorPoint=nullptr;
 require(component->queryInterface(IAudioProcessor::iid,
             reinterpret_cast<void**>(&audio))==kResultOk&&audio,"IAudioProcessor missing");
 require(component->queryInterface(IConnectionPoint::iid,
             reinterpret_cast<void**>(&processorPoint))==kResultOk&&processorPoint,
         "Processor connection point missing");
 IEditController* ctrl=nullptr;
 require(factory->createInstance(drumator::kControllerCID.toTUID(),IEditController::iid,
             reinterpret_cast<void**>(&ctrl))==kResultOk&&ctrl,"Controller factory");
 IConnectionPoint* controlPoint=nullptr;
 require(ctrl->queryInterface(IConnectionPoint::iid,
             reinterpret_cast<void**>(&controlPoint))==kResultOk&&controlPoint,
         "Controller connection point missing");
 HostApplication host;
 require(component->initialize(&host)==kResultOk,"Processor initialize");
 require(ctrl->initialize(&host)==kResultOk,"Controller initialize");
 // SO01: Studio One may display but refuse to activate kAux VST3
 // output buses. Verify advertised metadata, not only audio routing.
 require(component->getBusCount(kAudio,kOutput)==16,
         "SO01 expected 16 stereo output buses");
 for(Steinberg::int32 i=0;i<16;++i) {
   BusInfo info{};
   require(component->getBusInfo(kAudio,kOutput,i,info)==kResultOk,
           "SO01 getBusInfo failed");
   require(info.busType==BusTypes::kMain,
           "SO01 Studio One cannot activate kAux drum output");
   require(info.channelCount==2,"SO01 bus must be stereo");
   const bool defaultActive=(info.flags&BusInfo::kDefaultActive)!=0;
   require(defaultActive==(i==0),
           "SO01 only MAIN should be initially active");
   if(i==0) {
     require(info.name[0]==static_cast<TChar>('M')&&
             info.name[1]==static_cast<TChar>('A'),
             "SO01 MAIN name mismatch");
   }else{
     const int number=i+1;
     require(info.name[0]==static_cast<TChar>('D')&&
             info.name[4]==static_cast<TChar>(' ')&&
             info.name[5]==static_cast<TChar>('0'+number/10)&&
             info.name[6]==static_cast<TChar>('0'+number%10),
             "SO01 DRUM XX bus label mismatch");
   }
 }
 // Exercise the complete bus arrangement negotiation used by multi-output hosts.
 // This is distinct from activating an individual bus after a one-output setup.
 std::array<SpeakerArrangement,16> sixteenStereo{};
 sixteenStereo.fill(SpeakerArr::kStereo);
 require(audio->setBusArrangements(nullptr,0,sixteenStereo.data(),16)==kResultOk,
         "SO01 host negotiation for all 16 stereo outputs failed");
 auto invalidStereo=sixteenStereo;
 invalidStereo[15]=SpeakerArr::kMono;
 require(audio->setBusArrangements(nullptr,0,invalidStereo.data(),16)!=kResultOk,
         "SO01 accepted mono arrangement for DRUM 16");
 require(audio->setBusArrangements(nullptr,0,sixteenStereo.data(),16)==kResultOk,
         "SO01 failed to recover from rejected output arrangement");
 // SO02: a shorter bus array is not a valid *arrangement* request for this
 // fixed 16-bus plugin (optional activation does not remove any bus).
 // SDK requires kResultTrue only when the requested configuration matches
 // what getBusCount/getBusArrangement report afterwards.
 SpeakerArrangement oneStereo=SpeakerArr::kStereo;
 require(audio->setBusArrangements(nullptr,0,
           &oneStereo,1)!=kResultOk,
         "SO02 claimed successful negotiation for an unsupported 1-bus layout");
 for(int bus=0;bus<16;++bus) {
   SpeakerArrangement reported{};
   require(audio->getBusArrangement(kOutput,bus,reported)==kResultOk&&
           reported==SpeakerArr::kStereo,
           "SO02 rejected I/O layout damaged one of the 16 reported buses");
 }
 require(audio->setBusArrangements(nullptr,0,sixteenStereo.data(),16)==kResultOk,
         "SO02 full 16-bus layout failed after invalid negotiation");
 std::cout<<"SO01/SO02 16 kMain stereo outputs, fixed bus negotiation and recovery PASS\\n";
 // F16: the host must see semantic A-H pattern names, not normalized decimals.
 for(int i=0;i<8;++i) {
   String128 label{};
   require(ctrl->getParamStringByValue(
       drumator::kPatternParam,static_cast<double>(i)/7.0,label)==kResultOk,
       "Host pattern parameter string conversion failed");
   require(label[0]==static_cast<TChar>('A'+i)&&label[1]==0,
       "Host must display pattern letter A-H");
 }
 for(double v:{0.10,0.124,0.125,0.40,0.874,0.875,0.90,0.929,1.0}) {
   String128 label{};
   const auto expected=static_cast<int>(std::lround(v*7.0));
   require(ctrl->getParamStringByValue(drumator::kPatternParam,v,label)==kResultOk,
           "N05 host pattern display query");
   require(label[0]==static_cast<TChar>('A'+expected)&&label[1]==0&&
           static_cast<int>(drumator::patternFromNormalized(v))==expected,
           "N05 host and processor discrete pattern mismatch");
 }
 std::cout<<"N05 host/DSP noncanonical pattern values PASS\\n";
 std::cout<<"F16 host pattern labels A-H PASS\\n";

 // F15: host-owned controller state persists the user zoom preference.
 Steinberg::ResizableMemoryIBStream controllerZoom;
 require(ctrl->getState(&controllerZoom)==kResultOk&&
         controllerZoom.getCursor()==5+2*drumator::kRows,
         "F15 initial DZM3 zoom preference state");
 std::array<std::uint8_t,5> zoom150State{{0x44,0x5a,0x4d,0x31,1}};
 Steinberg::ResizableMemoryIBStream zoomRecall;
 Steinberg::int32 zoomBytes=0;
 require(zoomRecall.write(zoom150State.data(),5,&zoomBytes)==kResultOk&&
         zoomBytes==5,"F15 write controller zoom fixture");
 zoomRecall.rewind();
 require(ctrl->setState(&zoomRecall)==kResultOk,"F15 restore 150 percent zoom");
 Steinberg::ResizableMemoryIBStream zoomSaved;
 require(ctrl->getState(&zoomSaved)==kResultOk&&
         zoomSaved.getCursor()==5+2*drumator::kRows,
         "F15 DZM3 zoom preference failed roundtrip size");
 const auto* z=static_cast<const std::uint8_t*>(zoomSaved.getData());
 require(z[0]=='D'&&z[1]=='Z'&&z[2]=='M'&&z[3]=='3'&&z[4]==3,
         "F15 legacy DZM1 zoom recall was not upgraded to DZM3");
 std::cout<<"F15 controller zoom preference project-state PASS\\n";
 require(controlPoint->connect(processorPoint)==kResultOk,"Controller connection");
 require(processorPoint->connect(controlPoint)==kResultOk,"Processor connection");
 require(audio->setBusArrangements(nullptr,0,sixteenStereo.data(),16)==kResultOk,
         "Complete 16-bus stereo arrangement");
 ProcessSetup setup{};
 setup.sampleRate=48000;setup.maxSamplesPerBlock=256;
 setup.symbolicSampleSize=kSample32;setup.processMode=kRealtime;
 require(audio->setupProcessing(setup)==kResultOk,"setupProcessing 48 kHz");
 require(audio->getTailSamples()==kInfiniteTail,
         "Internal drum sequencer must request continuous host processing");
 require(component->setActive(true)==kResultOk,"setActive true");
 require(audio->setProcessing(true)==kResultOk,"setProcessing true");
 std::array<float,256> left{},right{};
 float* samples[2]{left.data(),right.data()};
 AudioBusBuffers bus{};bus.numChannels=2;bus.channelBuffers32=samples;
 EventList events(8);
 ProcessContext context{};context.state=0;
 ProcessData data{};
 data.numSamples=256;data.numOutputs=1;data.outputs=&bus;
 data.symbolicSampleSize=kSample32;data.processMode=kRealtime;
 data.inputEvents=&events;data.processContext=&context;
 auto run=[&](int note){
   events.clear();
   if(note>=0) {
     Event e{};e.type=Event::kNoteOnEvent;e.busIndex=0;
     e.sampleOffset=0;e.noteOn.pitch=static_cast<int16>(note);
     e.noteOn.velocity=1.f;e.noteOn.noteId=-1;
     require(events.addEvent(e)==kResultOk,"Add MIDI event");
   }
   left.fill(-99.f);right.fill(-99.f);
   require(audio->process(data)==kResultOk,"process() returned non-success");
 };
 run(36);
 for(int i=0;i<256;++i)
    require(left[i]==0.f&&right[i]==0.f,"Unloaded first row not silent");
 std::cout<<"Empty sampler silence PASS\n";
 auto send=[&](const std::vector<std::uint8_t>& bytes,int row)->tresult {
   HostMessage message;
   message.setMessageID("Drumator.LoadPcm16Wav");
   auto* attr=message.getAttributes();
   require(attr!=nullptr,"HostMessage attributes absent");
   require(attr->setInt("Row",row)==kResultOk,"Row attribute");
   constexpr const char* filename="StudioDrum.wav";
   require(attr->setBinary("SampleName",filename,14)==kResultOk,
           "Name attribute");
   require(attr->setBinary("WavBytes",bytes.data(),
                 static_cast<uint32>(bytes.size()))==kResultOk,"Binary attribute");
   return processorPoint->notify(&message);
 };
 auto audition=[&](int bits,bool floating,float amp){
   const auto wav=makeWav(bits,floating,amp);
   require(send(wav,0)==kResultOk,"Host WAV message rejected");
   require(audio->setProcessing(false)==kResultOk,"Stop processing");
   require(component->setActive(false)==kResultOk,"Deactivate");
   require(component->setActive(true)==kResultOk,"Reactivate");
   require(audio->setProcessing(true)==kResultOk,"Resume processing");
   run(36);
   double avgL=0.0,avgR=0.0;
   for(int i=50;i<190;++i) {
      require(std::isfinite(left[i])&&std::isfinite(right[i]),"NaN/Inf output");
      avgL+=left[i];avgR+=right[i];
   }
   avgL/=140.0;avgR/=140.0;
   std::cout<<"WAV PCM "<<bits<<(floating?" float":" int")
            <<" amplitude expected "<<amp<<" actual L "<<avgL<<" R "<<avgR<<'\n';
   require(std::abs(avgL-amp)<0.015&&std::abs(avgR-amp)<0.015,
           "Actual plugin WAV level mismatch");
 };
 audition(16,false,.5f);
 audition(24,false,.25f);
 audition(32,true,-.5f);
 // Real packaged VST3 GUI audition: validate message contract and actual
 // one-shot audio while transport is stopped (no MIDI input).
 auto preview=[&](int row) {
   HostMessage message;
   message.setMessageID("Drumator.PreviewRow");
   auto* attrs=message.getAttributes();
   require(attrs&&attrs->setInt("Row",row)==kResultOk,"Preview message attribute");
   return processorPoint->notify(&message);
 };
 context.state=0;
 for(int i=0;i<8;++i)run(-1); // drain previous MIDI voice
 require(preview(-1)!=kResultOk&&preview(16)!=kResultOk,
         "Preview accepted invalid row");
 require(preview(1)!=kResultOk,"Preview accepted empty row");
 require(preview(0)==kResultOk,"Loaded row preview rejected");
 run(-1);
 double previewMean=0.;
 for(int i=50;i<190;++i)previewMean+=left[i];
 require(std::abs(previewMean/140.+.5)<.015,
         "GUI preview was silent or wrong gain with transport stopped");
 for(int i=0;i<8;++i)run(-1);
 run(-1);
 for(int i=0;i<256;++i)
   require(left[i]==0.f&&right[i]==0.f,
           "Preview retriggered without a new request");
 require(preview(0)==kResultOk&&preview(0)==kResultOk,
         "Repeated preview request rejected");
 run(-1);
 double coalescedMean=0.;
 for(int i=50;i<190;++i)coalescedMean+=left[i];
 require(std::abs(coalescedMean/140.+.5)<.015,
         "Repeated preview requests stacked voices or went silent");
 for(int i=0;i<8;++i)run(-1);
 std::cout<<"Packaged GUI one-shot sample preview / stopped transport / invalid rows PASS\\n";
 // T22: an audition consumed during an outputless processing gap
 // must not play later when the host restores the MAIN bus.
 require(preview(0)==kResultOk,"T22 preview request");
 ProcessData noPreviewOutputs=data;
 noPreviewOutputs.numOutputs=0;
 noPreviewOutputs.outputs=nullptr;
 require(audio->process(noPreviewOutputs)==kResultOk,"T22 outputless block");
 run(-1);
 for(int i=0;i<256;++i)
   require(left[i]==0.f&&right[i]==0.f,
           "T22 deferred stale GUI preview after outputs restored");
 require(preview(0)==kResultOk,"T22 fresh preview request");
 run(-1);
 double t22Level=0.;
 for(int i=50;i<190;++i)t22Level+=std::abs(left[i]);
 require(t22Level>1.0,"T22 fresh preview lost after outputless gap");
 for(int i=0;i<8;++i)run(-1);
 std::cout<<"T22 outputless GUI preview lifetime PASS\\n";
 // Exercise actual packaged VST3 global master DSP at zero, half and double
 // gain, including invalid control values and return to neutral gain.
 auto setMaster=[&](int value)->tresult {
   HostMessage message;
   message.setMessageID("Drumator.MasterVolume");
   require(message.getAttributes()->setInt("Percent",value)==kResultOk,
           "Master percent host attribute");
   return processorPoint->notify(&message);
 };
 auto verifyMaster=[&](int amount,double expected) {
   require(setMaster(amount)==kResultOk,"Master control message rejected");
   for(int tail=0;tail<12;++tail)run(-1);
   run(36);
   double measured=0;
   for(int i=50;i<190;++i)measured+=left[i];
   require(std::abs(measured/140.-expected)<0.018,
           "Packaged VST3 master volume did not scale actual audio");
 };
 verifyMaster(0,0.0);
 verifyMaster(50,-.25);
 verifyMaster(200,-1.0);
 require(setMaster(201)!=kResultOk&&setMaster(-1)!=kResultOk,
         "Master volume accepted invalid percent");
 verifyMaster(100,-.5);
 // G02: live global gain edits do not hard-jump an active long sample.
 for(int i=0;i<5;++i)run(-1); // drain the previous amplitude fixture
 run(36); // original 1,024-frame constant PCM WAV, 256 frames played
 require(setMaster(200)==kResultOk,"G02 master up");
 run(-1);
 require(left[0]<-.49f&&left[0]>-.53f&&left[250]<-.97f&&left[250]>-1.03f,
         "G02 jumped immediately instead of de-clicking up");
 for(int i=1;i<240;++i)
   require(std::abs(left[i]-left[i-1])<.015f,"G02 upward gain discontinuity");
 require(setMaster(0)==kResultOk,"G02 master down");
 run(-1);
 require(left[0]<-.97f&&left[0]>-1.03f&&std::abs(left[250])<.001f,
         "G02 missing master down fade");
 for(int i=1;i<240;++i)
   require(std::abs(left[i]-left[i-1])<.015f,"G02 downward gain discontinuity");
 require(setMaster(100)==kResultOk,"G02 return to unity");
 for(int i=0;i<12;++i)run(-1);
 std::cout<<"G02 packaged VST3 master de-click up/down PASS\\n";
 // M18 packaged host: verify actual per-row mix, not only core math.
 auto rowMix=[&](int gain,int pan)->tresult {
   HostMessage msg;msg.setMessageID("Drumator.RowMix");
   auto* attrs=msg.getAttributes();
   require(attrs->setInt("Row",0)==kResultOk,"Mix Row attribute");
   require(attrs->setInt("Gain",gain)==kResultOk,"Mix Gain attribute");
   require(attrs->setInt("Pan",pan)==kResultOk,"Mix Pan attribute");
   return processorPoint->notify(&msg);
 };
 require(rowMix(50,-100)==kResultOk,"Mix half-gain left");
 for(int tail=0;tail<7;++tail)run(-1);
 run(36);
 double mixedL=0,mixedR=0;
 for(int i=50;i<190;++i){mixedL+=left[i];mixedR+=right[i];}
 require(std::abs(mixedL/140.+.25)<.015&&std::abs(mixedR/140.)<.015,
         "Packaged VST3 gain/pan mismatch");
 require(rowMix(100,0)==kResultOk,"Restore unity center");
 require(rowMix(201,0)!=kResultOk,"Overrange row gain accepted");
 for(int tail=0;tail<7;++tail)run(-1);
 auto bad=makeWav(16,false,.25f);bad[0]='X';
 require(send(bad,0)!=kResultOk,"Corrupt WAV accepted");
 run(36);
 double level=0;
 for(int i=50;i<190;++i)level+=left[i];
 require(level/140.0<-.4,"Rejected WAV destroyed previous sample");
 // Prior voice can continue until its end; drain old tail first.
 for(int n=0;n<7;++n)run(-1);
 run(37);
 for(int i=0;i<256;++i)
   require(left[i]==0.f&&right[i]==0.f,"Unloaded row 2 produced audio");
 std::cout<<"Invalid WAV preserved audio, unloaded row 2 silent PASS\n";
 // Save an actual initialized component via its public VST3 IBStream
 // interface. Decode the recorded bytes to prove assets are truly embedded.
 // Direct VST3 message contract: SONG A-B-A, three bars.
 {
   HostMessage config;
   config.setMessageID("Drumator.SongConfig");
   auto* a=config.getAttributes();
   const std::array<std::uint8_t,6> entries{0,16,1,16,0,16};
   require(a&&a->setInt("Enabled",1)==kResultOk&&
           a->setInt("Count",3)==kResultOk&&
           a->setBinary("Entries",entries.data(),
             static_cast<Steinberg::uint32>(entries.size()))==kResultOk,
           "SONG host message creation");
   require(processorPoint->notify(&config)==kResultOk,
           "SONG host message rejected");
 }
 // Real packaged VST3 messaging and DM13 persistence for global master and
 // per-row performance randomization (not merely a standalone codec test).
 {
   HostMessage master;
   master.setMessageID("Drumator.MasterVolume");
   require(master.getAttributes()->setInt("Percent",73)==kResultOk,
           "master attribute setup");
   require(processorPoint->notify(&master)==kResultOk,
           "master VST3 message rejected");
   HostMessage random;
   random.setMessageID("Drumator.RowRandomVelocity");
   require(random.getAttributes()->setInt("Row",2)==kResultOk&&
           random.getAttributes()->setInt("Percent",45)==kResultOk,
           "random velocity attribute setup");
   require(processorPoint->notify(&random)==kResultOk,
           "random velocity VST3 message rejected");
   HostMessage invalid;
   invalid.setMessageID("Drumator.RowRandomVelocity");
   require(invalid.getAttributes()->setInt("Row",16)==kResultOk&&
           invalid.getAttributes()->setInt("Percent",45)==kResultOk,
           "invalid random velocity setup");
   require(processorPoint->notify(&invalid)!=kResultOk,
           "out-of-range random velocity row accepted");
 }
 // DM14: rename the instrument without modifying its original WAV filename.
 {
   HostMessage rename;
   rename.setMessageID("Drumator.RenameRow");
   auto* attr=rename.getAttributes();
   constexpr char label[]="Closed Hat";
   require(attr&&attr->setInt("Row",0)==kResultOk&&
           attr->setBinary("DisplayName",label,sizeof(label)-1)==kResultOk&&
           processorPoint->notify(&rename)==kResultOk,
           "DM14 valid row label rejected");
   HostMessage reject;
   reject.setMessageID("Drumator.RenameRow");
   auto* invalidNameAttr=reject.getAttributes();
   const std::string oversized(65,'X');
   require(invalidNameAttr&&invalidNameAttr->setInt("Row",0)==kResultOk&&
           invalidNameAttr->setBinary("DisplayName",oversized.data(),
                          static_cast<uint32>(oversized.size()))==kResultOk&&
           processorPoint->notify(&reject)!=kResultOk,
           "DM14 invalid row label accepted");
 }
 Steinberg::ResizableMemoryIBStream saved;
 require(component->getState(&saved)==kResultOk,"VST3 getState failed");
 const auto* raw=static_cast<const std::uint8_t*>(saved.getData());
 drumator::KitState savedKit;
 require(drumator::KitCodec::decode(raw,saved.getCursor(),savedKit),
         "Complete DMR4 kit not saved");
 require(savedKit.wav[0]==makeWav(32,true,-.5f),
         "Original FLOAT32 WAV was not embedded in song state");
 require(savedKit.names[0]=="StudioDrum.wav","WAV filename missing from song");
 require(savedKit.displayNames[0]=="Closed Hat","DM14 custom display name missing");
 require(savedKit.masterVolume==73&&savedKit.randomVelocity[2]==45&&
         savedKit.randomVelocity[0]==0,
         "DM13 master/random host messages not persisted");
 require(savedKit.visible==4,"Default four visible rows lost");
 require(savedKit.songEnabled==1&&savedKit.chain.count()==3&&
         savedKit.chain.entry(0).pattern==0&&
         savedKit.chain.entry(1).pattern==1&&
         savedKit.chain.entry(2).pattern==0&&
         savedKit.chain.entry(0).steps==16&&
         savedKit.chain.entry(1).steps==16&&
         savedKit.chain.entry(2).steps==16,
         "SONG A-B-A persistence from actual VST3 failed");
 // Return temporary performance modifiers to unity/zero so subsequent
 // legacy audio amplitude smoke tests remain isolated.
 {
   HostMessage master;
   master.setMessageID("Drumator.MasterVolume");
   require(master.getAttributes()->setInt("Percent",100)==kResultOk&&
           processorPoint->notify(&master)==kResultOk,
           "master reset after persistence check");
   HostMessage random;
   random.setMessageID("Drumator.RowRandomVelocity");
   require(random.getAttributes()->setInt("Row",2)==kResultOk&&
           random.getAttributes()->setInt("Percent",0)==kResultOk&&
           processorPoint->notify(&random)==kResultOk,
           "random velocity reset after persistence check");
 }
 // Restore PATTERN mode after the SONG-specific persistence check.
 // Subsequent legacy host tests explicitly require manual A-H selection.
 {
   HostMessage config;
   config.setMessageID("Drumator.SongConfig");
   auto* attributes=config.getAttributes();
   const std::array<std::uint8_t,6> entries{0,16,1,16,0,16};
   require(attributes&&attributes->setInt("Enabled",0)==kResultOk&&
           attributes->setInt("Count",3)==kResultOk&&
           attributes->setBinary("Entries",entries.data(),
             static_cast<Steinberg::uint32>(entries.size()))==kResultOk,
           "SONG OFF host message construction");
   require(processorPoint->notify(&config)==kResultOk,
           "SONG OFF before manual pattern regressions");
 }
 // Create a SECOND fresh plugin instance: restore state BEFORE setupProcessing,
 // as real hosts commonly do when reloading a project.
 IComponent* restored=nullptr;
 require(factory->createInstance(drumator::kProcessorCID.toTUID(),IComponent::iid,
              reinterpret_cast<void**>(&restored))==kResultOk&&restored,
         "Second VST3 processor instance");
 IAudioProcessor* restoredAudio=nullptr;
 require(restored->queryInterface(IAudioProcessor::iid,
              reinterpret_cast<void**>(&restoredAudio))==kResultOk&&restoredAudio,
         "Second VST3 audio interface");
 IEditController* restoredController=nullptr;
 require(factory->createInstance(drumator::kControllerCID.toTUID(),
              IEditController::iid,reinterpret_cast<void**>(&restoredController))
             ==kResultOk&&restoredController,"Second controller");
 require(restored->initialize(&host)==kResultOk,"Second processor initialize");
 require(restoredController->initialize(&host)==kResultOk,
         "Second controller initialize");
 saved.rewind();
 require(restored->setState(&saved)==kResultOk,"Second processor setState");
 saved.rewind();
 require(restoredController->setComponentState(&saved)==kResultOk,
         "Second controller setComponentState with embedded kit");
 require(restoredAudio->setBusArrangements(nullptr,0,sixteenStereo.data(),16)==kResultOk,
         "Second stereo bus");
 require(restoredAudio->setupProcessing(setup)==kResultOk,
         "Second setupProcessing must reprepare embedded WAV");
 require(restored->setActive(true)==kResultOk,"Second activate");
 require(restoredAudio->setProcessing(true)==kResultOk,"Second setProcessing true");
 events.clear();
 Event restoredNote{};restoredNote.type=Event::kNoteOnEvent;
 restoredNote.noteOn.pitch=36;restoredNote.noteOn.velocity=1.f;
 restoredNote.noteOn.noteId=-1;
 require(events.addEvent(restoredNote)==kResultOk,"Second MIDI note event");
 left.fill(-99.f);right.fill(-99.f);
 require(restoredAudio->process(data)==kResultOk,"Second audio render");
 double restoredMean=0;
 for(int i=50;i<190;++i)restoredMean+=left[i];
 // DM13 restores the saved 73% master gain: -0.5 * 0.73 = -0.365.
 require(std::abs(restoredMean/140.+.365)<0.015,
         "Project reload did not reproduce FLOAT32 WAV audio with saved master gain");
 require(restoredAudio->setProcessing(false)==kResultOk,"Second stop processing");
 require(restored->setActive(false)==kResultOk,"Second deactivate");
 require(restoredController->terminate()==kResultOk,"Second controller terminate");
 require(restored->terminate()==kResultOk,"Second component terminate");
 restoredAudio->release();restoredController->release();restored->release();
 std::cout<<"DM13 fresh-instance audio and 73% master recall PASS\n";
 // Restore neutral gain in the original host instance before legacy
 // per-sample amplitude checks; the saved DM13 project above is unchanged.
 {
   HostMessage neutralMaster;
   neutralMaster.setMessageID("Drumator.MasterVolume");
   require(neutralMaster.getAttributes()->setInt("Percent",100)==kResultOk&&
           processorPoint->notify(&neutralMaster)==kResultOk,
           "Reset master to unity for subsequent isolated host tests");
 }

 // M11: exercise all 16 independent sample slots through the packaged VST3.
 // Retain unique WAV bytes per row, verify each MIDI note and full song state.
 for(int row=0;row<16;++row) {
   const float amplitude=0.10f+static_cast<float>(row)*0.025f;
   require(send(makeWav(32,true,amplitude),row)==kResultOk,
           "Full kit load rejected by processor");
   run(36+row);
   double measured=0;
   for(int i=50;i<190;++i) {
     require(std::isfinite(left[i])&&std::isfinite(right[i]),
             "Full kit audio produced NaN or Inf");
     measured+=left[i];
   }
   require(std::abs(measured/140.-amplitude)<0.02,
           "Full kit MIDI row playback amplitude mismatch");
   for(int tail=0;tail<5;++tail)run(-1);
 }
 Steinberg::ResizableMemoryIBStream fullState;
 require(component->getState(&fullState)==kResultOk,
         "Full 16-row getState failed");
 drumator::KitState fullKit;
 require(drumator::KitCodec::decode(
           static_cast<const std::uint8_t*>(fullState.getData()),
           fullState.getCursor(),fullKit),
         "Full kit encoded state failed CRC or WAV validation");
 for(const auto bus:fullKit.output)
   require(bus==0,"V1FIX freshly built kit must keep every instrument on MAIN");
 for(int row=0;row<16;++row)
   require(fullKit.wav[static_cast<std::size_t>(row)]==
            makeWav(32,true,0.10f+static_cast<float>(row)*0.025f),
           "Full 16-row kit lost embedded WAV bytes");
 // M12: restore a FULL kit into a separate processor, before setupProcessing,
 // and verify that every original WAV actually plays (not just serialized).
 IComponent* kitCopy=nullptr;
 require(factory->createInstance(drumator::kProcessorCID.toTUID(),
             IComponent::iid,reinterpret_cast<void**>(&kitCopy))==kResultOk&&kitCopy,
         "Full-kit fresh processor");
 IAudioProcessor* kitAudio=nullptr;
 require(kitCopy->queryInterface(IAudioProcessor::iid,
             reinterpret_cast<void**>(&kitAudio))==kResultOk&&kitAudio,
         "Full-kit audio interface");
 require(kitCopy->initialize(&host)==kResultOk,"Full-kit initialize");
 fullState.rewind();
 require(kitCopy->setState(&fullState)==kResultOk,"Full-kit restore state");
 require(kitAudio->setBusArrangements(nullptr,0,sixteenStereo.data(),16)==kResultOk,
         "Full-kit stereo arrangement");
 require(kitAudio->setupProcessing(setup)==kResultOk,
         "Full-kit setupProcessing");
 require(kitCopy->setActive(true)==kResultOk,"Full-kit activate");
 require(kitAudio->setProcessing(true)==kResultOk,"Full-kit start processing");
 for(int row=0;row<16;++row) {
   events.clear();
   Event note{};note.type=Event::kNoteOnEvent;note.noteOn.pitch=
       static_cast<int16>(36+row);
   note.noteOn.velocity=1.f;note.noteOn.noteId=-1;
   require(events.addEvent(note)==kResultOk,"Full-kit add note");
   left.fill(0.f);right.fill(0.f);
   require(kitAudio->process(data)==kResultOk,"Full-kit render");
   const double expected=0.10+static_cast<double>(row)*0.025;
   double avgL=0,avgR=0;
   for(int n=50;n<190;++n){avgL+=left[n];avgR+=right[n];}
   require(std::abs(avgL/140.-expected)<0.02&&
           std::abs(avgR/140.-expected)<0.02,
           "Fresh instance failed to play a restored drum row");
   events.clear();
   for(int tail=0;tail<5;++tail) {
     left.fill(0.f);right.fill(0.f);
     require(kitAudio->process(data)==kResultOk,
             "Full-kit tail rendering");
   }
 }
 require(kitAudio->setProcessing(false)==kResultOk,"Full-kit stop");
 require(kitCopy->setActive(false)==kResultOk,"Full-kit deactivate");
 require(kitCopy->terminate()==kResultOk,"Full-kit terminate");
 kitAudio->release();kitCopy->release();
 std::cout<<"M12 fresh-instance all-16-sample audio recall PASS\\n";
 // M27: merely enabling a stem must NOT reroute freshly created rows:
 // all instruments remain on MAIN until the user explicitly selects OUT.
 require(component->activateBus(kAudio,kOutput,1,true)==kResultOk,
         "Activate OUT 2 stereo stem");
 std::array<float,256> stemLeft{},stemRight{};
 float* stemChannels[2]{stemLeft.data(),stemRight.data()};
 AudioBusBuffers stemBus{};
 stemBus.numChannels=2;stemBus.channelBuffers32=stemChannels;
 AudioBusBuffers routedBuses[2]{bus,stemBus};
 data.numOutputs=2;data.outputs=routedBuses;
 for(int n=0;n<6;++n)run(-1);
 run(37);
 double mainSum=0,stemSum=0;
 for(int n=50;n<190;++n){mainSum+=left[n];stemSum+=stemLeft[n];}
 require(std::abs(mainSum/140.-.125)<.02&&std::abs(stemSum)<.01,
         "V1FIX activating OUT 2 changed untouched MAIN default");
 // The previous M27/M30 fixture assumed row N was automatically assigned
 // to OUT N. Under the new product contract explicitly perform the same
 // manual assignments via the REAL packaged processor message API.
 auto setOutput=[&](int row,int selectedBus)->tresult {
   HostMessage msg;msg.setMessageID("Drumator.RowOutput");
   auto* attrs=msg.getAttributes();
   require(attrs&&attrs->setInt("Row",row)==kResultOk&&
           attrs->setInt("Bus",selectedBus)==kResultOk,
           "V1FIX manual output-select message attributes");
   return processorPoint->notify(&msg);
 };
 require(setOutput(1,1)==kResultOk&&setOutput(2,2)==kResultOk,
         "V1FIX manual OUT 2/3 assignment rejected");
 for(int n=0;n<6;++n)run(-1);
 run(37);
 mainSum=0;stemSum=0;
 for(int n=50;n<190;++n) {
    mainSum+=left[n];stemSum+=stemLeft[n];
 }
 require(std::abs(mainSum)<0.01&&std::abs(stemSum/140.-.125)<.02,
         "M27 active stem leaked to main or produced wrong audio");
 require(component->activateBus(kAudio,kOutput,1,false)==kResultOk,
         "Deactivate row 2 stereo stem");
 for(int n=0;n<6;++n)run(-1);
 run(37);
 mainSum=0;
 for(int n=50;n<190;++n)mainSum+=left[n];
 require(std::abs(mainSum/140.-.125)<.02,
         "M27 inactive stem did not fall back to main stereo bus");
 data.numOutputs=1;data.outputs=&bus;
 std::cout<<"M27 active and inactive VST3 stereo stem routing PASS\\n";
 // M30: two active stems simultaneously, plus an inactive row on MAIN.
 require(component->activateBus(kAudio,kOutput,1,true)==kResultOk,"Stem 2 activate");
 require(component->activateBus(kAudio,kOutput,2,true)==kResultOk,"Stem 3 activate");
 std::array<float,256> secondL{},secondR{};
 float* secondChannels[2]{secondL.data(),secondR.data()};
 AudioBusBuffers secondBus{};
 secondBus.numChannels=2;secondBus.channelBuffers32=secondChannels;
 AudioBusBuffers threeBuses[3]{bus,stemBus,secondBus};
 data.numOutputs=3;data.outputs=threeBuses;
 for(int n=0;n<7;++n)run(-1);
 events.clear();
 for(int pitch : {36,37,38}) {
   Event note{};note.type=Event::kNoteOnEvent;note.busIndex=0;
   note.sampleOffset=0;note.noteOn.pitch=static_cast<int16>(pitch);
   note.noteOn.velocity=1.f;note.noteOn.noteId=-1;
   require(events.addEvent(note)==kResultOk,"Simultaneous MIDI event");
 }
 left.fill(0);right.fill(0);stemLeft.fill(0);stemRight.fill(0);
 secondL.fill(0);secondR.fill(0);
 require(audio->process(data)==kResultOk,"Three-bus render");
 auto average=[](const std::array<float,256>& a) {
   double v=0;for(int n=50;n<190;++n)v+=a[n];return v/140.;
 };
 require(std::abs(average(left)-.10)<.02&&
         std::abs(average(stemLeft)-.125)<.02&&
         std::abs(average(secondL)-.15)<.02,
         "Simultaneous main/stem levels or isolation incorrect");
 require(std::abs(average(right)-.10)<.02&&
         std::abs(average(stemRight)-.125)<.02&&
         std::abs(average(secondR)-.15)<.02,
         "Simultaneous stereo outputs mismatch");
 events.clear();
 require(component->activateBus(kAudio,kOutput,2,false)==kResultOk,
         "Stem 3 deactivate");
 for(int n=0;n<7;++n)run(-1);
 run(38);
 require(std::abs(average(left)-.15)<.02&&
         std::abs(average(stemLeft))<.01,
         "Disabled stem 3 did not return exclusively to MAIN");
 require(component->activateBus(kAudio,kOutput,1,false)==kResultOk,
         "Stem 2 deactivate");
 data.numOutputs=1;data.outputs=&bus;
 for(int n=0;n<7;++n)run(-1);
 std::cout<<"M30 simultaneous three-instrument multi-out routing PASS\\n";
 // Invalid MIDI pitches and zero velocity must not trigger audio.
 for(int n=0;n<7;++n)run(-1);
 for(int pitch : {20,35,52,127}) {
   run(pitch);
   for(int i=0;i<256;++i)
     require(left[i]==0.f&&right[i]==0.f,
             "Out-of-range MIDI note produced sound");
 }
 events.clear();
 Event zero{};
 zero.type=Event::kNoteOnEvent;
 zero.noteOn.pitch=36;
 zero.noteOn.velocity=0.f;
 zero.noteOn.noteId=-1;
 require(events.addEvent(zero)==kResultOk,"Zero velocity MIDI event");
 left.fill(9.f);right.fill(9.f);
 require(audio->process(data)==kResultOk,"Process zero velocity");
 for(int i=0;i<256;++i)
   require(left[i]==0.f&&right[i]==0.f,
           "Zero velocity MIDI note produced sound");
 events.clear();
 // MIDI event offset must be sample-accurate in the packaged VST3.
 // An offset of 128 frames must not create audio earlier in this block.
 for(int tail=0;tail<7;++tail)run(-1);
 events.clear();
 Event offsetNote{};
 offsetNote.type=Event::kNoteOnEvent;
 offsetNote.busIndex=0;
 offsetNote.sampleOffset=128;
 offsetNote.noteOn.pitch=36;
 offsetNote.noteOn.velocity=1.f;
 offsetNote.noteOn.noteId=-1;
 require(events.addEvent(offsetNote)==kResultOk,"Offset note add");
 left.fill(-99.f);right.fill(-99.f);
 require(audio->process(data)==kResultOk,"Offset MIDI process");
 for(int n=0;n<128;++n)
    require(left[n]==0.f&&right[n]==0.f,
            "MIDI onset rendered before its sample offset");
 double delayedEnergy=0;
 for(int n=178;n<244;++n)delayedEnergy+=std::abs(left[n]);
 require(delayedEnergy>1.0,"MIDI onset missing at specified sample offset");
 events.clear();
 for(int tail=0;tail<7;++tail)run(-1);
 std::cout<<"MIDI 128-frame sample-accurate onset PASS\\n";
 // Packaged host control-contract QA: instrument pitch, stem destination
 // and visible row count must reject invalid values and survive VST3 save.
 {
   auto sendRowInt=[&](const char* id,const char* key,int row,int value)->tresult {
     HostMessage m;m.setMessageID(id);
     auto* a=m.getAttributes();
     require(a&&a->setInt("Row",row)==kResultOk&&
             a->setInt(key,value)==kResultOk,"Row control attributes");
     return processorPoint->notify(&m);
   };
   auto setVisible=[&](int count)->tresult {
     HostMessage m;m.setMessageID("Drumator.VisibleRows");
     require(m.getAttributes()->setInt("Count",count)==kResultOk,
             "Visible row count attribute");
     return processorPoint->notify(&m);
   };
   require(sendRowInt("Drumator.RowPitch","Semitones",0,7)==kResultOk,
           "Pitch +7 control rejected");
   require(sendRowInt("Drumator.RowPitch","Semitones",0,25)!=kResultOk&&
           sendRowInt("Drumator.RowPitch","Semitones",16,0)!=kResultOk,
           "Invalid row pitch accepted");
   require(sendRowInt("Drumator.RowOutput","Bus",0,5)==kResultOk,
           "Stem destination 5 rejected");
   require(sendRowInt("Drumator.RowOutput","Bus",0,16)!=kResultOk,
           "Invalid stem bus accepted");
   require(setVisible(8)==kResultOk&&
           setVisible(0)!=kResultOk&&setVisible(17)!=kResultOk,
           "Visible row count bounds invalid");
   Steinberg::ResizableMemoryIBStream controls;
   require(component->getState(&controls)==kResultOk,
           "Pitch/stem/visible state read failed");
   drumator::KitState state{};
   require(drumator::KitCodec::decode(
          static_cast<const std::uint8_t*>(controls.getData()),
          controls.getCursor(),state)&&
          state.pitch[0]==31&&state.output[0]==5&&state.visible==8,
          "Pitch/stem/visible controls not serialized");
   require(sendRowInt("Drumator.RowPitch","Semitones",0,0)==kResultOk&&
           sendRowInt("Drumator.RowOutput","Bus",0,0)==kResultOk&&
           setVisible(4)==kResultOk,
           "Pitch/stem/visible neutral reset failed");
   std::cout<<"Packaged VST3 pitch, stem destination and visible-row state PASS\\n";
 }
 // Batched host regression: editable steps, whole-pattern paste/copy,
 // bounds checking, and persistence through the actual VST3 component.
 {
   auto editStep=[&](int pat,int row,int step,int velocity)->tresult {
     HostMessage msg;msg.setMessageID("Drumator.StepEdit");
     auto* a=msg.getAttributes();
     require(a&&a->setInt("Pattern",pat)==kResultOk&&
             a->setInt("Row",row)==kResultOk&&
             a->setInt("Step",step)==kResultOk&&
             a->setInt("Velocity",velocity)==kResultOk,"StepEdit attributes");
     return processorPoint->notify(&msg);
   };
   auto replacePattern=[&](int pat,const std::array<std::uint8_t,drumator::kRows*drumator::kSteps>& values)->tresult {
     HostMessage msg;msg.setMessageID("Drumator.PatternReplace");
     auto* a=msg.getAttributes();
     require(a&&a->setInt("Pattern",pat)==kResultOk&&
             a->setBinary("Steps",values.data(),
                 static_cast<Steinberg::uint32>(values.size()))==kResultOk,
             "PatternReplace attributes");
     return processorPoint->notify(&msg);
   };
   require(editStep(7,15,31,91)==kResultOk,
           "StepEdit last row/last step rejected");
   require(editStep(8,15,31,91)!=kResultOk&&
           editStep(7,16,31,91)!=kResultOk&&
           editStep(7,15,32,91)!=kResultOk&&
           editStep(7,15,31,128)!=kResultOk,
           "StepEdit accepted out-of-range coordinates/velocity");
   std::array<std::uint8_t,drumator::kRows*drumator::kSteps> target{};
   target[0]=127;target[31]=1;target[15*drumator::kSteps+31]=76;
   require(replacePattern(6,target)==kResultOk,"Pattern paste rejected");
   require(replacePattern(8,target)!=kResultOk,"Out-of-range pattern paste accepted");
   const auto baseline=target;
   target[0]=255;
   require(replacePattern(6,target)!=kResultOk,"Invalid velocity paste accepted");
   Steinberg::ResizableMemoryIBStream snap;
   require(component->getState(&snap)==kResultOk,"Editable pattern state capture");
   drumator::KitState state{};
   require(drumator::KitCodec::decode(
     static_cast<const std::uint8_t*>(snap.getData()),snap.getCursor(),state),
     "Editable pattern state decode");
   require(state.grid[drumator::AtomicPatternBank::index(7,15,31)]==91&&
           state.grid[drumator::AtomicPatternBank::index(6,0,0)]==127&&
           state.grid[drumator::AtomicPatternBank::index(6,0,31)]==1&&
           state.grid[drumator::AtomicPatternBank::index(6,15,31)]==76,
           "StepEdit/paste failed VST3 state persistence or invalid paste corrupted pattern");
   std::array<std::uint8_t,drumator::kRows*drumator::kSteps> blank{};
   require(replacePattern(6,blank)==kResultOk&&
           replacePattern(7,blank)==kResultOk,
           "Pattern paste regression cleanup");
   std::cout<<"Packaged VST3 StepEdit/PatternReplace, boundaries and state PASS\\n";
 }
 // Batched VST3 control regressions: mute/solo/choke message boundaries,
 // independent row state, persistence, and cleanup for following audio tests.
 {
   auto gate=[&](int row,int solo,int enabled)->tresult {
     HostMessage msg;msg.setMessageID("Drumator.RowGate");
     auto* a=msg.getAttributes();
     require(a&&a->setInt("Row",row)==kResultOk&&
             a->setInt("Solo",solo)==kResultOk&&
             a->setInt("On",enabled)==kResultOk,"Gate attributes");
     return processorPoint->notify(&msg);
   };
   auto choke=[&](int row,int group)->tresult {
     HostMessage msg;msg.setMessageID("Drumator.RowChoke");
     auto* a=msg.getAttributes();
     require(a&&a->setInt("Row",row)==kResultOk&&
             a->setInt("Group",group)==kResultOk,"Choke attributes");
     return processorPoint->notify(&msg);
   };
   require(gate(3,0,1)==kResultOk&&gate(5,1,1)==kResultOk&&
           choke(3,4)==kResultOk&&choke(5,1)==kResultOk,
           "Valid mute/solo/choke controls rejected");
   require(gate(16,0,1)!=kResultOk&&gate(3,2,1)!=kResultOk&&
           gate(3,0,2)!=kResultOk&&gate(-1,1,1)!=kResultOk&&
           choke(16,2)!=kResultOk&&choke(3,5)!=kResultOk&&
           choke(3,-1)!=kResultOk,
           "Mute/solo/choke accepted invalid control value");
   Steinberg::ResizableMemoryIBStream snapshot;
   require(component->getState(&snapshot)==kResultOk,
           "Gate/choke state capture");
   drumator::KitState state{};
   require(drumator::KitCodec::decode(
     static_cast<const std::uint8_t*>(snapshot.getData()),
     snapshot.getCursor(),state),
     "Gate/choke state decode");
   require(state.muted==(std::uint16_t{1}<<3)&&
           state.solo==(std::uint16_t{1}<<5)&&
           state.choke[3]==4&&state.choke[5]==1,
           "Mute/solo/choke VST3 state not persisted independently");
   require(gate(3,0,0)==kResultOk&&gate(5,1,0)==kResultOk&&
           choke(3,0)==kResultOk&&choke(5,0)==kResultOk,
           "Gate/choke cleanup failed");
   std::cout<<"Packaged VST3 mute/solo/choke bounds and state PASS\\n";
 }
 // Functional QA: control messages must update actual saved component state.
 auto percentage=[&](const char* id,int number)->tresult {
   HostMessage message;message.setMessageID(id);
   auto* attrs=message.getAttributes();
   require(attrs->setInt("Percent",number)==kResultOk,"Percent attribute");
   return processorPoint->notify(&message);
 };
 require(percentage("Drumator.Swing",35)==kResultOk,"Swing 35 rejected");
 require(percentage("Drumator.Humanize",70)==kResultOk,"Humanize 70 rejected");
 require(percentage("Drumator.Swing",51)!=kResultOk,"Invalid swing accepted");
 require(percentage("Drumator.Humanize",101)!=kResultOk,
         "Invalid humanize accepted");
 Steinberg::ResizableMemoryIBStream controlsSaved;
 require(component->getState(&controlsSaved)==kResultOk,
         "GetState after humanize and swing failed");
 drumator::KitState controlsKit;
 require(drumator::KitCodec::decode(
         static_cast<const std::uint8_t*>(controlsSaved.getData()),
         controlsSaved.getCursor(),controlsKit),
         "Controls state failed to decode");
 require(controlsKit.swing==35&&controlsKit.humanize==70,
         "Swing/Humanize actual VST3 persistence mismatch");
 // Test recall through a distinct VST3 processor instance, not only codec.
 IComponent* controlCopy=nullptr;
 require(factory->createInstance(drumator::kProcessorCID.toTUID(),
           IComponent::iid,reinterpret_cast<void**>(&controlCopy))==kResultOk&&controlCopy,
         "Control recall fresh processor factory");
 require(controlCopy->initialize(&host)==kResultOk,"Control recall initialize");
 controlsSaved.rewind();
 require(controlCopy->setState(&controlsSaved)==kResultOk,
         "Control recall fresh processor setState");
 Steinberg::ResizableMemoryIBStream roundtripControls;
 require(controlCopy->getState(&roundtripControls)==kResultOk,
         "Control recall fresh processor getState");
 drumator::KitState roundtripKit;
 require(drumator::KitCodec::decode(
         static_cast<const std::uint8_t*>(roundtripControls.getData()),
         roundtripControls.getCursor(),roundtripKit),
         "Control recall serialized state invalid");
 require(roundtripKit.swing==35&&roundtripKit.humanize==70,
         "Swing/humanize changed after fresh processor restoration");
 require(controlCopy->terminate()==kResultOk,"Control recall terminate");
 controlCopy->release();
 require(percentage("Drumator.Swing",0)==kResultOk,"Swing reset failed");
 require(percentage("Drumator.Humanize",0)==kResultOk,"Humanize reset failed");
 std::cout<<"VST3 swing/humanize controls validation and persistence PASS\\n";
 // Functional transport QA on the actual VST3 processor.
 context.tempo=120.0;
 context.projectTimeMusic=0.0;
 context.state=ProcessContext::kPlaying|
               ProcessContext::kProjectTimeMusicValid|
               ProcessContext::kTempoValid;
 run(-1);
 double transportSound=0;
 for(int i=50;i<190;++i)transportSound+=std::abs(left[i]);
 require(transportSound>1.0,"Valid playing host clock did not trigger sequencer");
 context.state=0;
 run(-1);
 for(int i=0;i<256;++i)
   require(left[i]==0.f&&right[i]==0.f,
           "Transport stop did not silence sequencer tails");
 context.state=ProcessContext::kPlaying; // missing PPQ and tempo validity
 run(-1);
 for(int i=0;i<256;++i)
   require(left[i]==0.f&&right[i]==0.f,
           "Playing without valid tempo/PPQ invented sequencer events");
 context.state=0;
 run(36);
 double auditionLevel=0;
 for(int i=50;i<190;++i)auditionLevel+=std::abs(left[i]);
 require(auditionLevel>1.0,
         "MIDI audition stopped working while host transport is stopped");
 std::cout<<"VST3 stop, invalid clock and stopped MIDI audition PASS\\n";
 // Pattern-bank QA: real host playback after project recall of two patterns.
 // A contains a hit at step zero; B has no hits at any step.
 drumator::KitState patternKit=controlsKit;
 patternKit.selected=0;
 for(std::size_t row=0;row<drumator::kRows;++row)
   for(std::size_t step=0;step<drumator::kSteps;++step) {
     patternKit.grid[drumator::AtomicPatternBank::index(0,row,step)]=0;
     patternKit.grid[drumator::AtomicPatternBank::index(1,row,step)]=0;
   }
 patternKit.grid[drumator::AtomicPatternBank::index(0,0,0)]=127;
 std::vector<std::uint8_t> patternBytes;
 require(drumator::KitCodec::encode(patternKit,patternBytes),"Pattern kit encode");
 auto loadPattern=[&](std::uint8_t selected){
   patternKit.selected=selected;
   require(drumator::KitCodec::encode(patternKit,patternBytes),"Pattern encode");
   require(audio->setProcessing(false)==kResultOk,"Pattern stop processing");
   require(component->setActive(false)==kResultOk,"Pattern deactivate");
   Steinberg::ResizableMemoryIBStream stream;
   Steinberg::int32 actualWritten=0;
   require(stream.write(patternBytes.data(),static_cast<Steinberg::int32>(patternBytes.size()),
                        &actualWritten)==kResultOk&&
           actualWritten==static_cast<Steinberg::int32>(patternBytes.size()),
           "Pattern write stream");
   stream.rewind();
   require(component->setState(&stream)==kResultOk,"Pattern recall");
   require(component->setActive(true)==kResultOk,"Pattern activate");
   require(audio->setProcessing(true)==kResultOk,"Pattern resume");
 };
 // Transport regression: a fresh start at DAW bar 2 (PPQ 4), a locate
 // to bar 3, and a backwards loop must all restart pattern A at step zero.
 {
   const auto savedSwing=patternKit.swing;
   const auto savedHumanize=patternKit.humanize;
   const auto savedSong=patternKit.songEnabled;
   patternKit.swing=0;patternKit.humanize=0;patternKit.songEnabled=0;
   loadPattern(0);
   context.state=0;
   run(-1);
   context.state=ProcessContext::kPlaying|
                 ProcessContext::kProjectTimeMusicValid|
                 ProcessContext::kTempoValid;
   context.tempo=120.;
   for(double ppq : {4.,8.,4.}) {
     context.projectTimeMusic=ppq;
     run(-1);
     double downbeat=0.;
     for(int i=50;i<190;++i)downbeat+=std::abs(left[i]);
     require(downbeat>1.0,
             "Transport bar-2 start/locate/backwards loop missed first pattern step");
   }
   // T25: stop and restart at the same PPQ, without an intervening locate.
   // The transport origin must re-arm and replay the first step.
   context.state=0;
   run(-1);
   context.state=ProcessContext::kPlaying|
                 ProcessContext::kProjectTimeMusicValid|
                 ProcessContext::kTempoValid;
   context.projectTimeMusic=4.;
   run(-1);
   double samePositionRestart=0.;
   for(int i=50;i<190;++i)samePositionRestart+=std::abs(left[i]);
   require(samePositionRestart>1.0,
           "T25 stop/restart at same PPQ missed first sequencer step");
   // T16: an outputless processing gap must not strand the transport
   // or prevent a fresh downbeat when audio outputs return.
   context.projectTimeMusic=4.;
   auto noOutputBlock=data;
   noOutputBlock.numOutputs=0;
   noOutputBlock.outputs=nullptr;
   require(audio->process(noOutputBlock)==kResultOk,
           "T16 outputless transport gap rejected");
   context.projectTimeMusic=4.;
   run(-1);
   double resumedDownbeat=0.;
   for(int i=50;i<190;++i)resumedDownbeat+=std::abs(left[i]);
   require(resumedDownbeat>1.0,
           "T16 missing first step on output restoration");
   // The host may claim valid clock flags while supplying NaN values.
   // Drain the preceding one-shot completely without stopping transport,
   // then confirm the first step sounds on the next fractional-beat locate.
   context.projectTimeMusic=0.;
   run(-1);
   context.projectTimeMusic=std::numeric_limits<double>::quiet_NaN();
   context.tempo=std::numeric_limits<double>::quiet_NaN();
   for(int block=0;block<6;++block)run(-1);
   context.projectTimeMusic=4.25; // Second 16th at bar 2: origin step 17.
   context.tempo=120.;
   run(-1);
   double resumedHit=0.;
   for(int i=50;i<190;++i)resumedHit+=std::abs(left[i]);
   require(resumedHit>1.0,
           "Invalid host clock poisoned subsequent transport locate/first step");
   context.state=0;
   run(-1);
   patternKit.swing=savedSwing;patternKit.humanize=savedHumanize;
   patternKit.songEnabled=savedSong;
 }
 std::cout<<"VST3 transport bar-2 start, forward locate and loop restart PASS\\n";
 // DM15 independent A-H one/two-bar playback at a continuous 48k clock.
 // Step 20 remains stored when a pattern loops at step 16.
 {
   // The preceding control-contract test deliberately left swing=35% and
   // humanize=70% active. Isolate loop-length timing from that randomized
   // displacement, then restore the original modifiers below.
   const auto savedSwing=patternKit.swing;
   const auto savedHumanize=patternKit.humanize;
   patternKit.swing=0;patternKit.humanize=0;
   patternKit.songEnabled=0;
   const auto originalGrid=patternKit.grid;
   // Distinct nonzero tail markers across all A-H and rows. Keep A step 16
   // silent because the independent audio test measures that exact downbeat.
   for(std::size_t p=0;p<drumator::kPatterns;++p)
     for(std::size_t r=0;r<drumator::kRows;++r)
       for(std::size_t step=(p==0?17u:16u);step<32;++step)
         patternKit.grid[drumator::AtomicPatternBank::index(p,r,step)]=
           static_cast<std::uint8_t>(1+(p*37+r*11+step*3)%127);
   patternKit.patternLengths[0]=16;
   patternKit.grid[drumator::AtomicPatternBank::index(0,0,20)]=95;
   loadPattern(0);
   context.tempo=120.0;
   context.state=ProcessContext::kPlaying|
                 ProcessContext::kProjectTimeMusicValid|
                 ProcessContext::kTempoValid;
   double oneBar=0.;
   for(int block=0;block<=375;++block) {
     context.projectTimeMusic=static_cast<double>(block)*256.0*120.0/(60.0*48000.0);
     run(-1);
     if(block==375)for(int n=50;n<190;++n)oneBar+=std::abs(left[n]);
   }
   require(oneBar>1.,"DM15 1T failed to restart at bar 2");
   patternKit.patternLengths[0]=32;
   loadPattern(0);
   double twoBar=0.;
   for(int block=0;block<=375;++block) {
     context.projectTimeMusic=static_cast<double>(block)*256.0*120.0/(60.0*48000.0);
     run(-1);
     if(block==375)for(int n=50;n<190;++n)twoBar+=std::abs(left[n]);
   }
   require(twoBar<0.01,"DM15 2T restarted after 1 bar");
   context.state=0;
   // Control-message boundary and host serialization of changed A-H lengths.
   auto changeLength=[&](int pattern,int length)->tresult {
     HostMessage request;
     request.setMessageID("Drumator.PatternLength");
     auto* a=request.getAttributes();
     require(a&&a->setInt("Pattern",pattern)==kResultOk&&
             a->setInt("Steps",length)==kResultOk,
             "DM15 pattern length host attributes");
     return processorPoint->notify(&request);
   };
   require(changeLength(-1,16)!=kResultOk&&
           changeLength(8,16)!=kResultOk&&
           changeLength(0,8)!=kResultOk&&
           changeLength(0,0)!=kResultOk,
           "DM15 accepted invalid length/target");
   require(changeLength(0,16)==kResultOk,"DM15 control length 16 rejected");
   Steinberg::ResizableMemoryIBStream oneBarSaved;
   require(component->getState(&oneBarSaved)==kResultOk,
           "DM15 save after host control");
   drumator::KitState captured;
   require(drumator::KitCodec::decode(
       static_cast<const std::uint8_t*>(oneBarSaved.getData()),
       oneBarSaved.getCursor(),captured)&&captured.patternLengths[0]==16,
       "DM15 control edit did not persist to project");
   // T01: compare the returned component state, never the input fixture.
   auto tailPreserved=[&](const drumator::KitState& actual) {
     for(std::size_t p=0;p<drumator::kPatterns;++p)
       for(std::size_t r=0;r<drumator::kRows;++r)
         for(std::size_t step=16;step<32;++step) {
           const auto i=drumator::AtomicPatternBank::index(p,r,step);
           if(actual.grid[i]!=patternKit.grid[i])return false;
         }
     return true;
   };
   require(tailPreserved(captured),"T01 1T discarded stored steps 17-32");
   require(changeLength(0,32)==kResultOk,"DM15 restore length 32 rejected");
   Steinberg::ResizableMemoryIBStream twoBarSaved;
   require(component->getState(&twoBarSaved)==kResultOk&&
       drumator::KitCodec::decode(static_cast<const std::uint8_t*>(twoBarSaved.getData()),
           twoBarSaved.getCursor(),captured)&&captured.patternLengths[0]==32&&
       tailPreserved(captured),"T01 2T recall lost steps 17-32");
   auto erasedTail=captured;
   erasedTail.grid[drumator::AtomicPatternBank::index(0,0,20)]=0;
   require(!tailPreserved(erasedTail),"T01 negative control accepted lost step 20");
   drumator::qa::result("T01 PASS: actual returned state after 2T/1T/2T, all A-H/rows tail cells and erased-step negative");
   std::cout<<"T01 returned DM15 state: all A-H steps 17-32 and erase-negative PASS\n";
   patternKit.grid=originalGrid;
   patternKit.swing=savedSwing;patternKit.humanize=savedHumanize;
   std::cout<<"DM15 independent 1T/2T pattern loops and preserved steps PASS\n";
 }
 // Actual packaged VST3 SONG playback, not merely scheduler simulation:
 // three successive bars must sound A (loud), B (quiet), A (loud).
 {
   patternKit.grid[drumator::AtomicPatternBank::index(1,0,0)]=64;
   patternKit.songEnabled=1;
   require(patternKit.chain.setCount(3)&&
           patternKit.chain.configure(0,0,16)&&
           patternKit.chain.configure(1,1,16)&&
           patternKit.chain.configure(2,0,16),
           "Packaged song ABA configuration");
   loadPattern(0);
   context.tempo=120.;
   context.state=ProcessContext::kPlaying|
                 ProcessContext::kProjectTimeMusicValid|
                 ProcessContext::kTempoValid;
   std::array<double,3> barLevels{};
   for(std::uint32_t frame=0;frame<288000;frame+=256){
     context.projectTimeMusic=static_cast<double>(frame)/24000.0;
     run(-1);
     // Inspect every callback, not only the exact downbeat block: a
     // sample-accurate trigger may occur in the adjacent host buffer.
     double magnitude=0;
     for(int n=0;n<256;++n)magnitude+=std::abs(left[n]);
     const auto bar=static_cast<std::size_t>(frame/96000);
     if(magnitude/256.0>barLevels[bar])barLevels[bar]=magnitude/256.0;
   }
   std::cout<<"PACKAGED ABA measured levels: A="<<barLevels[0]
            <<" B="<<barLevels[1]<<" A="<<barLevels[2]<<'\\n';
   require(barLevels[0]>0.03&&barLevels[1]>0.01&&barLevels[2]>0.03,
           "Actual packaged VST3 ABA song omitted a bar onset");
   require(barLevels[1]<barLevels[0]*0.75&&
           std::abs(barLevels[2]-barLevels[0])<0.08,
           "Actual packaged VST3 ABA audio failed pattern A/B/A levels");
   patternKit.songEnabled=0;
   patternKit.grid[drumator::AtomicPatternBank::index(1,0,0)]=0;
   std::cout<<"Packaged VST3 three-bar ABA audio playback PASS\\n";
 }
 // Actual packaged VST3 per-row random velocity: 0% must be neutral,
 // 100% must alter audio without modifying the persisted step matrix.
 loadPattern(0);
 auto rowRandom=[&](int percent)->tresult {
   HostMessage msg;msg.setMessageID("Drumator.RowRandomVelocity");
   auto* attr=msg.getAttributes();
   require(attr->setInt("Row",0)==kResultOk&&
           attr->setInt("Percent",percent)==kResultOk,
           "Random audio test message");
   return processorPoint->notify(&msg);
 };
 auto measureRandom=[&](int percent)->double {
   require(rowRandom(percent)==kResultOk,"Random audio control rejected");
   context.state=0;run(-1);
   context.tempo=120.;
   context.projectTimeMusic=0.;
   context.state=ProcessContext::kPlaying|
                 ProcessContext::kProjectTimeMusicValid|
                 ProcessContext::kTempoValid;
   run(-1);
   double energy=0;
   for(int n=50;n<190;++n)energy+=std::abs(left[n]);
   return energy/140.;
 };
 const auto zeroRandom=measureRandom(0);
 const auto fullRandom=measureRandom(100);
 require(zeroRandom>0.05&&std::abs(fullRandom-zeroRandom)>0.0001,
         "Packaged VST3 random velocity did not change rendered audio");
 require(std::abs(measureRandom(0)-zeroRandom)<0.0001,
         "Random velocity zero did not restore original audio");
 Steinberg::ResizableMemoryIBStream randomSaved;
 require(component->getState(&randomSaved)==kResultOk,
         "Random audio state capture");
 drumator::KitState randomKit{};
 require(drumator::KitCodec::decode(
     static_cast<const std::uint8_t*>(randomSaved.getData()),
     randomSaved.getCursor(),randomKit),
     "Random audio state decoding");
 require(randomKit.grid[drumator::AtomicPatternBank::index(0,0,0)]==127,
         "Realtime random velocity overwrote stored step velocity");
 std::cout<<"Packaged VST3 realtime random velocity audio neutrality and persistence PASS\\n";
 loadPattern(0);
 context.tempo=120.;
 context.projectTimeMusic=0.;
 context.state=ProcessContext::kPlaying|
               ProcessContext::kProjectTimeMusicValid|
               ProcessContext::kTempoValid;
 run(-1);
 double aEnergy=0;
 for(int n=50;n<190;++n)aEnergy+=std::abs(left[n]);
 require(aEnergy>1.0,"Pattern A recall did not produce step 1 audio");
 loadPattern(1);
 context.projectTimeMusic=0.;
 run(-1);
 for(int n=0;n<256;++n)
   require(left[n]==0.f&&right[n]==0.f,
           "Pattern B silent grid unexpectedly produced audio");
 // Switch patterns during playback without stopping/activating the processor.
 // Use the host's real parameter-change queue at successive step boundaries.
 loadPattern(0);
 ParameterChanges patternChanges;
 data.inputParameterChanges=&patternChanges;
 auto switchPattern=[&](int pattern) {
   patternChanges.clearQueue();
   int32 index=0;
   auto* queue=patternChanges.addParameterData(drumator::kPatternParam,index);
   require(queue!=nullptr,"Pattern parameter queue creation");
   int32 point=0;
   require(queue->addPoint(0,static_cast<double>(pattern)/7.,point)==kResultOk,
           "Pattern automation event");
 };
 context.projectTimeMusic=0.;
 switchPattern(0);
 run(-1);
 double firstHit=0;
 for(int n=50;n<190;++n)firstHit+=std::abs(left[n]);
 require(firstHit>1.0,"Playing pattern A initial hit missing");
 context.projectTimeMusic=.25;
 switchPattern(1);
 // Switching patterns must not cut an already ringing one-shot.
 // The sound loaded in row 1 has 1024 frames; drain it while B remains
 // selected and the host clock advances continuously.
 run(-1);
 for(int block=1;block<=6;++block) {
   context.projectTimeMusic=.25+
      static_cast<double>(block*256)*context.tempo/(60.0*48000.0);
   run(-1);
 }
 for(int n=0;n<256;++n)
   require(left[n]==0.f&&right[n]==0.f,
           "Silent pattern B emitted new events after previous voice decayed");
 context.projectTimeMusic=0.;
 switchPattern(0);
 run(-1);
 double returnedHit=0;
 for(int n=50;n<190;++n)returnedHit+=std::abs(left[n]);
 require(returnedHit>1.0,"Playing pattern A resume did not sound");
 // M36: a program change at sample 64 must affect a step at sample 128
 // without retroactively changing the first half of the audio block.
 patternKit.grid[drumator::AtomicPatternBank::index(0,0,0)]=0;
 patternKit.grid[drumator::AtomicPatternBank::index(1,0,1)]=127;
 loadPattern(0);
 context.tempo=5625.;
 context.projectTimeMusic=0.;
 patternChanges.clearQueue();
 {
   int32 index=0,point=0;
   auto* queue=patternChanges.addParameterData(drumator::kPatternParam,index);
   require(queue!=nullptr&&queue->addPoint(64,1./7.,point)==kResultOk,
           "Mid-block pattern change event");
 }
 run(-1);
 for(int n=0;n<128;++n)
   require(left[n]==0.f&&right[n]==0.f,
           "Mid-block pattern B was applied before scheduled step 1");
 double midBlockHit=0.;
 for(int n=180;n<240;++n)midBlockHit+=std::abs(left[n]);
 require(midBlockHit>1.,"Mid-block program change missed step at sample 128");
 patternChanges.clearQueue();
 context.tempo=120.;
 std::cout<<"M36 sample-offset pattern switch within host block PASS\\n";
 // Packaged host proof: MIDI note 25 selects B at sample 64 and the
 // second sequencer step at sample 128 plays B, never before offset.
 auto midiSelect=[&](int pitch,int offset) {
   events.clear();
   Event e{};e.type=Event::kNoteOnEvent;e.busIndex=0;
   e.sampleOffset=offset;e.noteOn.pitch=static_cast<int16>(pitch);
   e.noteOn.velocity=1.f;e.noteOn.noteId=-1;
   require(events.addEvent(e)==kResultOk,"MIDI program note");
   left.fill(-99.f);right.fill(-99.f);
   require(audio->process(data)==kResultOk,"MIDI selection audio block");
 };
 loadPattern(0);context.projectTimeMusic=0.;context.tempo=5625.;
 patternChanges.clearQueue();
 data.inputParameterChanges=nullptr;
 midiSelect(25,64);
 for(int n=0;n<128;++n)
   require(left[n]==0.f&&right[n]==0.f,"MIDI pattern selection emitted an early hit");
 double midiMagnitude=0.;
 for(int n=180;n<240;++n)midiMagnitude+=std::abs(left[n]);
 require(midiMagnitude>1.,"MIDI pattern B missed frame-128 hit");
 loadPattern(0);context.projectTimeMusic=0.;
 midiSelect(24,64);
 for(int n=0;n<256;++n)
   require(left[n]==0.f&&right[n]==0.f,"MIDI pattern A unexpectedly sounded B");
 // At the same sample offset a host automation point takes precedence.
 loadPattern(0);context.projectTimeMusic=0.;
 patternChanges.clearQueue();
 { int32 index=0,point=0;
   auto* q=patternChanges.addParameterData(drumator::kPatternParam,index);
   require(q&&q->addPoint(64,1./7.,point)==kResultOk,
           "Automation/MIDI collision event");
 }
 data.inputParameterChanges=&patternChanges;
 midiSelect(24,64);
 double hostMagnitude=0.;
 for(int n=180;n<240;++n)hostMagnitude+=std::abs(left[n]);
 require(hostMagnitude>1.,"Host automation failed to override MIDI at equal offset");
 std::cout<<"MIDI A-H / automation ordering packaged audio PASS\n";
 // A01: processor MIDI A->B reports a host output parameter at offset64.
 {
   ParameterChanges published;
   data.outputParameterChanges=&published;
   loadPattern(0);context.projectTimeMusic=0.;context.tempo=5625.;
   require(ctrl->setParamNormalized(drumator::kPatternParam,0.)==kResultOk,
           "A01 initial controller pattern");
   data.inputParameterChanges=nullptr;
   midiSelect(25,64);
   require(published.getParameterCount()==1,"A01 missing output parameter");
   auto* q=published.getParameterData(0);
   require(q&&q->getParameterId()==drumator::kPatternParam&&q->getPointCount()==1,
           "A01 wrong parameter output queue");
   int32 offset=0;ParamValue value=0;
   require(q->getPoint(0,offset,value)==kResultTrue&&offset==64&&
           std::abs(value-1./7.)<1e-9,"A01 wrong sample offset/value");
   require(ctrl->setParamNormalized(drumator::kPatternParam,value)==kResultOk&&
           std::abs(ctrl->getParamNormalized(drumator::kPatternParam)-1./7.)<1e-9,
           "A01 controller did not follow host MIDI feedback");
   data.outputParameterChanges=nullptr;
 }
 // A02: 250 distinct host events + 8 MIDI changes exceeded the old 256 cap.
 {
   loadPattern(0);context.projectTimeMusic=0.;context.tempo=5625.;
   ParameterChanges flood;int32 parameterIndex=0;
   auto* q=flood.addParameterData(drumator::kPatternParam,parameterIndex);
   require(q!=nullptr,"A02 host queue");
   for(int offset=0;offset<250;++offset){
     int32 pointIndex=0;
     require(q->addPoint(offset,1./7.,pointIndex)==kResultOk,
             "A02 host automation addPoint");
   }
   events.clear();
   for(int offset=0;offset<8;++offset) {
     Event e{};e.type=Event::kNoteOnEvent;e.busIndex=0;
     e.sampleOffset=offset;e.noteOn.pitch=24;e.noteOn.velocity=1.f;e.noteOn.noteId=-1;
     require(events.addEvent(e)==kResultOk,"A02 MIDI addEvent");
   }
   data.inputParameterChanges=&flood;
   require(audio->process(data)==kResultOk,"A02 host process 258 events");
   double measured=0.;
   for(int n=180;n<240;++n)measured+=std::abs(left[n]);
   require(measured>1.,"A02 B missing after >256 events");
   data.inputParameterChanges=nullptr;
   events.clear();
 }
 context.tempo=120.;
 data.inputParameterChanges=nullptr;
 context.state=0;
 std::cout<<"VST3 distinct pattern recall and rendered playback PASS\\n";
 // A deterministic extended callback workload; this is NOT a paced,
 // real-time deadline or Studio One endurance test.
 for(int block=0;block<8192;++block) {
   run(block%16==0?36+(block/16)%16:-1);
   for(int n=0;n<256;++n)
     require(std::isfinite(left[n])&&std::isfinite(right[n]),
             "Extended processing encountered nonfinite output");
 }
 std::cout<<"M11 full 16-row packaged audio and 8192-block finite-output PASS\n";
 // M25: exercise M24 sample removal through the actual packaged VST3.
 // Prior one-shots are drained; all other rows must remain playable.
 HostMessage clear;
 clear.setMessageID("Drumator.ClearSample");
 require(clear.getAttributes()->setInt("Row",3)==kResultOk,
         "Clear row attribute");
 require(processorPoint->notify(&clear)==kResultOk,
         "Remove sample from row 4 failed");
 for(int i=0;i<5;++i)run(-1);
 run(39);
 for(int n=0;n<256;++n)
   require(left[n]==0.f&&right[n]==0.f,
           "Removed sample row still emitted audio");
 run(40);
 double neighbor=0.0;
 for(int n=50;n<190;++n)neighbor+=left[n];
 require(std::abs(neighbor/140.-0.20)<0.02,
         "Removing row 4 damaged neighboring row 5");
 Steinberg::ResizableMemoryIBStream clearedState;
 require(component->getState(&clearedState)==kResultOk,
         "Save after clearing sample failed");
 drumator::KitState clearedKit;
 require(drumator::KitCodec::decode(
            static_cast<const std::uint8_t*>(clearedState.getData()),
            clearedState.getCursor(),clearedKit),
         "Cleared kit failed state decode");
 require(clearedKit.wav[3].empty()&&clearedKit.names[3].empty()&&
         clearedKit.wav[4]==makeWav(32,true,.20f),
         "Removed WAV persisted or unaffected row vanished");
 std::cout<<"M25 actual VST3 sample removal/audio/state QA PASS\\n";
 // M34: transactional project recall through the actual VST3 component.
 Steinberg::ResizableMemoryIBStream goodState;
 require(component->getState(&goodState)==kResultOk,
         "Save before malicious host state");
 const auto* savedData=static_cast<const std::uint8_t*>(goodState.getData());
 const auto savedCount=static_cast<std::size_t>(goodState.getCursor());
 require(savedCount>100,"Saved kit unexpectedly short");
 const std::vector<std::uint8_t> originalState(savedData,savedData+savedCount);
 for(int fault=0;fault<3;++fault) {
   auto damaged=originalState;
   if(fault==0)damaged[90]^=0x40;
   if(fault==1)damaged.back()^=0x20;
   if(fault==2)damaged.resize(25);
   Steinberg::ResizableMemoryIBStream corrupted;
   Steinberg::int32 written=0;
   require(corrupted.write(damaged.data(),static_cast<Steinberg::int32>(damaged.size()),
                           &written)==kResultOk&&
           written==static_cast<Steinberg::int32>(damaged.size()),
           "Write corrupt state");
   corrupted.rewind();
   require(component->setState(&corrupted)!=kResultOk,
           "Invalid project state accepted by VST3");
   Steinberg::ResizableMemoryIBStream afterFailure;
   require(component->getState(&afterFailure)==kResultOk,
           "Save after failed recall");
   require(static_cast<std::size_t>(afterFailure.getCursor())==originalState.size()&&
           std::memcmp(afterFailure.getData(),originalState.data(),
                       originalState.size())==0,
           "Failed VST3 recall changed the active kit");
 }
 std::cout<<"M34 VST3 corrupt/truncated state rejection and rollback PASS\\n";
 // R26: successful recall of an unchanged, valid project must be
 // idempotent and preserve the entire portable kit byte-for-byte.
 {
   Steinberg::ResizableMemoryIBStream repeatedState;
   Steinberg::int32 repeatWritten=0;
   auto writableState=originalState;
   require(repeatedState.write(writableState.data(),
           static_cast<Steinberg::int32>(originalState.size()),
           &repeatWritten)==kResultOk&&
           repeatWritten==static_cast<Steinberg::int32>(originalState.size()),
           "R26 write valid kit state");
   repeatedState.rewind();
   require(component->setState(&repeatedState)==kResultOk,
           "R26 valid kit second recall rejected");
   Steinberg::ResizableMemoryIBStream repeatSaved;
   require(component->getState(&repeatSaved)==kResultOk,
           "R26 save after repeated recall");
   require(static_cast<std::size_t>(repeatSaved.getCursor())==originalState.size()&&
           std::memcmp(repeatSaved.getData(),originalState.data(),
                       originalState.size())==0,
           "R26 repeated valid recall changed serialized kit");
 }
 std::cout<<"R26 repeated valid VST3 project recall PASS\\n";
 // M26: real FLAC fixture, 128 frames of 0.25 PCM16 at 48 kHz.
 const std::uint8_t flacFixture[]{0x66,0x4c,0x61,0x43,0x00,0x00,0x00,0x22,0x10,0x00,0x10,0x00,0x00,0x00,0x0c,0x00,0x00,0x0c,0x0b,0xb8,0x00,0xf0,0x00,0x00,0x00,0x80,0xb1,0x44,0xc4,0xb3,0x94,0xfd,0x9c,0xff,0xe7,0x8c,0x33,0xf1,0x9d,0x64,0xa1,0x61,0x84,0x00,0x00,0x28,0x20,0x00,0x00,0x00,0x72,0x65,0x66,0x65,0x72,0x65,0x6e,0x63,0x65,0x20,0x6c,0x69,0x62,0x46,0x4c,0x41,0x43,0x20,0x31,0x2e,0x34,0x2e,0x33,0x20,0x32,0x30,0x32,0x33,0x30,0x36,0x32,0x33,0x00,0x00,0x00,0x00,0xff,0xf8,0x6a,0x08,0x00,0x7f,0x5d,0x01,0x00,0x09,0x04,0x3a};
 std::vector<std::uint8_t> imported;
 require(drumator::flacToWav(flacFixture,sizeof(flacFixture),imported),
         "Real FLAC fixture refused by import decoder");
 drumator::DecodedWav flacDecoded;
 require(drumator::WavPcm16Decoder::decode(imported.data(),imported.size(),
                                         flacDecoded)==drumator::WavError::Ok,
         "FLAC-to-WAV conversion produced invalid WAV");
 require(flacDecoded.sampleRate==48000&&flacDecoded.channels==1&&
         flacDecoded.left.size()==128&&
         std::abs(flacDecoded.left[64]-.25f)<.001f,
         "FLAC decode sample amplitude or metadata mismatch");
 auto brokenFlac=std::vector<std::uint8_t>(
    flacFixture,flacFixture+sizeof(flacFixture));
 brokenFlac[0]=0;
 std::vector<std::uint8_t> retainedImport=imported;
 require(!drumator::flacToWav(brokenFlac.data(),brokenFlac.size(),imported)&&
         imported==retainedImport,"Broken FLAC modified prior output");
 // F13: compressed FLAC metadata can advertise an enormous decoded size.
 // Reject before a large PCM allocation, preserving the previous output.
 auto oversizedFlac=std::vector<std::uint8_t>(
    flacFixture,flacFixture+sizeof(flacFixture));
 // STREAMINFO's total-samples field occupies the low 36 bits in bytes 21-25.
 oversizedFlac[21]=static_cast<std::uint8_t>(oversizedFlac[21]|0x0f);
 oversizedFlac[22]=0xff;oversizedFlac[23]=0xff;
 oversizedFlac[24]=0xff;oversizedFlac[25]=0xff;
 require(!drumator::flacToWav(oversizedFlac.data(),oversizedFlac.size(),imported)&&
         imported==retainedImport,
         "F13 oversized FLAC metadata allocated or changed output");
 std::cout<<"F13 oversized compressed FLAC guard PASS\\n";
 auto shortFlac=std::vector<std::uint8_t>(
     flacFixture,flacFixture+sizeof(flacFixture));
 shortFlac[21]=static_cast<std::uint8_t>(shortFlac[21]&0xf0);
 shortFlac[22]=0;shortFlac[23]=0;
 shortFlac[24]=1;shortFlac[25]=0; // declared 256, actual 128
 require(!drumator::flacToWav(shortFlac.data(),shortFlac.size(),imported)&&
         imported==retainedImport,"N03 truncated FLAC accepted");
 std::cout<<"N03 FLAC declared-frame mismatch rejected PASS\\n";
 require(send(retainedImport,0)==kResultOk,
         "FLAC-derived WAV rejected by packaged VST3");
 for(int i=0;i<7;++i)run(-1);
 run(36);
 double fromFlac=0;
 for(int i=30;i<100;++i)fromFlac+=left[i];
 require(std::abs(fromFlac/70.-.25)<.015,
         "FLAC-derived sample played at wrong amplitude");
 std::cout<<"M26 real FLAC decoder and packaged VST3 playback PASS\\n";
 // M35: a host changes sample rate after samples are already loaded.
 // The processor must rebuild prepared sample assets and resume audition.
 require(audio->setProcessing(false)==kResultOk,"Rate switch stop");
 require(component->setActive(false)==kResultOk,"Rate switch deactivate");
 setup.sampleRate=44100;
 context.sampleRate=setup.sampleRate;
 require(audio->setupProcessing(setup)==kResultOk,
         "Host sample rate 48k to 44.1k setup");
 // T28: after a rate switch the next valid host clock must establish
 // a fresh phase origin, rather than reuse the preceding sample-rate clock.
 require(audio->getTailSamples()==kInfiniteTail,
         "Generator tail changed after host sample-rate switch");
 require(component->setActive(true)==kResultOk,"Rate switch activate");
 require(audio->setProcessing(true)==kResultOk,"Rate switch resume");
 context.state=0;
 for(int n=0;n<9;++n)run(-1);
 run(36);
 double rateChangedAudio=0;
 for(int n=30;n<100;++n) {
   require(std::isfinite(left[n])&&std::isfinite(right[n]),
           "Rate switch produced invalid audio");
   rateChangedAudio+=left[n];
 }
 require(std::abs(rateChangedAudio/70.-.25)<.025,
         "Previously loaded sample failed after host rate switch");
 std::cout<<"M35 VST3 48k to 44.1k host rate switch playback PASS\\n";
 auditRateSwitchClock(component,audio,setup);
 // F03: verify the public host parameter/recall path on the live controller.
 require(ctrl->setParamNormalized(drumator::kPatternParam,3.0/7.0)==kResultOk,
         "F03 host-driven parameter update rejected");
 require(std::abs(ctrl->getParamNormalized(drumator::kPatternParam)-3.0/7.0)<1e-9,
         "F03 controller lost host pattern update");
 Steinberg::ResizableMemoryIBStream controllerRecallState;
 require(component->getState(&controllerRecallState)==kResultOk,
         "F03 state snapshot");
 controllerRecallState.rewind();
 require(ctrl->setComponentState(&controllerRecallState)==kResultOk,
         "F03 controller state recall");
 drumator::KitState controllerRecallKit;
 require(drumator::KitCodec::decode(
           static_cast<const std::uint8_t*>(controllerRecallState.getData()),
           controllerRecallState.getCursor(),controllerRecallKit),
         "F03 recalled kit decode");
 require(std::abs(ctrl->getParamNormalized(drumator::kPatternParam)-
                  static_cast<double>(controllerRecallKit.selected)/7.0)<1e-9,
         "F03 controller parameter not synchronized after recall");
 // F03: create/destroy editor views around host-driven parameter updates
 // and state recall. Checks controller/editor lifetime without a native HWND.
 for(int repeat=0;repeat<3;++repeat) {
   auto* view=ctrl->createView(ViewType::kEditor);
   require(view!=nullptr,"F03 editor reopen returned null");
   ViewRect bounds{};
   require(view->getSize(&bounds)==kResultOk&&
           bounds.right-bounds.left==1280,
           "F03 reopened editor geometry");
   require(ctrl->setParamNormalized(drumator::kPatternParam,
            static_cast<double>(repeat)/7.0)==kResultOk,
           "F03 parameter update with live editor object");
   view->release();
 }
 std::cout<<"F03 host controller state and parameter recall PASS\\n";
 // T17: a malformed negative host frame count must be rejected before
 // any silence, scheduling, or buffer-clearing operation.
 {
   ProcessData invalidFrames{};
   invalidFrames.numSamples=-1;
   invalidFrames.numOutputs=0;
   invalidFrames.outputs=nullptr;
   invalidFrames.symbolicSampleSize=kSample32;
   invalidFrames.processMode=kRealtime;
   require(audio->process(invalidFrames)==kInvalidArgument,
           "T17 negative frame count not rejected");
 }
 std::cout<<"T17 negative host frame count rejection PASS\\n";
 // T19: after setupProcessing(maxSamplesPerBlock=256), an oversized
 // host block must be rejected before any buffer access or restore gate.
 {
   ProcessData oversized{};
   oversized.numSamples=257;
   oversized.numOutputs=0;
   oversized.outputs=nullptr;
   oversized.symbolicSampleSize=kSample32;
   oversized.processMode=kRealtime;
   require(audio->process(oversized)==kInvalidArgument,
           "T19 oversized host block not rejected");
 }
 std::cout<<"T19 oversized host frame count rejection PASS\\n";
 // T20: reject malformed output-bus metadata before accessing any
 // host buffers, including while the restore gate is active.
 {
   ProcessData badOutputMetadata{};
   badOutputMetadata.numSamples=256;
   badOutputMetadata.symbolicSampleSize=kSample32;
   badOutputMetadata.processMode=kRealtime;
   badOutputMetadata.numOutputs=-1;
   require(audio->process(badOutputMetadata)==kInvalidArgument,
           "T20 negative bus count not rejected");
   badOutputMetadata.numOutputs=17;
   require(audio->process(badOutputMetadata)==kInvalidArgument,
           "T20 excessive bus count not rejected");
   badOutputMetadata.numOutputs=1;
   badOutputMetadata.outputs=nullptr;
   require(audio->process(badOutputMetadata)==kInvalidArgument,
           "T20 missing output array not rejected");
 }
 std::cout<<"T20 malformed output bus metadata rejection PASS\\n";
 // T21: even the restore-silence path must not trust a host-provided
 // channel count beyond the plugin's fixed stereo output contract.
 {
   AudioBusBuffers malformedBus{};
   ProcessData malformedChannels{};
   malformedChannels.numSamples=256;
   malformedChannels.numOutputs=1;
   malformedChannels.outputs=&malformedBus;
   malformedChannels.symbolicSampleSize=kSample32;
   malformedChannels.processMode=kRealtime;
   malformedBus.numChannels=-1;
   require(audio->process(malformedChannels)==kInvalidArgument,
           "T21 negative channel count not rejected");
   malformedBus.numChannels=3;
   require(audio->process(malformedChannels)==kInvalidArgument,
           "T21 excess channel count not rejected");
   malformedBus.numChannels=1;
   require(audio->process(malformedChannels)==kInvalidArgument,
           "T23 mono channel count on a stereo bus not rejected");
 }
 std::cout<<"T21 malformed output channel metadata rejection PASS\\n";

 // F04: hosts flush automation using zero sample frames and no output bus.
 // The saved component state must reflect the last flushed pattern.
 ParameterChanges flushChanges;
 int32 flushQueueIndex=0;
 auto* flushQueue=flushChanges.addParameterData(
     drumator::kPatternParam,flushQueueIndex);
 require(flushQueue!=nullptr,"F04 flush queue");
 int32 flushPointIndex=0;
 require(flushQueue->addPoint(0,5.0/7.0,flushPointIndex)==kResultOk,
         "F04 flush point");
 ProcessData flushData{};
 flushData.numSamples=0;
 flushData.numOutputs=0;
 flushData.outputs=nullptr;
 flushData.inputParameterChanges=&flushChanges;
 flushData.symbolicSampleSize=kSample32;
 flushData.processMode=kRealtime;
 require(audio->process(flushData)==kResultOk,
         "F04 host zero-frame automation flush");
 Steinberg::ResizableMemoryIBStream flushedState;
 require(component->getState(&flushedState)==kResultOk,
         "F04 post-flush state");
 drumator::KitState flushedKit;
 require(drumator::KitCodec::decode(
             static_cast<const std::uint8_t*>(flushedState.getData()),
             flushedState.getCursor(),flushedKit),
         "F04 state decode");
 require(flushedKit.selected==5,
         "F04 zero-frame automation lost the selected pattern");
 // N05: actual packaged processor must select the host's displayed pattern
 // even when an automation point lies between the canonical p/7 values.
 for(const double v:{0.10,0.124,0.125,0.875,0.90,0.929}) {
   flushChanges.clearQueue();
   int32 idx=0;
   auto* q=flushChanges.addParameterData(drumator::kPatternParam,idx);
   require(q!=nullptr,"N05 flush automation queue");
   int32 point=0;
   require(q->addPoint(0,v,point)==kResultOk,"N05 automation point");
   require(audio->process(flushData)==kResultOk,"N05 actual host automation");
   Steinberg::ResizableMemoryIBStream updated;
   require(component->getState(&updated)==kResultOk,"N05 state save");
   drumator::KitState observed;
   require(drumator::KitCodec::decode(
               static_cast<const std::uint8_t*>(updated.getData()),
               updated.getCursor(),observed),"N05 state decode");
   require(observed.selected==drumator::patternFromNormalized(v),
           "N05 real processor selected different pattern from host");
 }
 std::cout<<"N05 actual processor/host noncanonical automation PASS\\n";
 std::cout<<"F04 zero-frame host automation state PASS\\n";
 // F05: a nonzero audio block with no output buses still carries
 // sample-offset MIDI A-H changes. Host automation wins same-offset ties.
 flushData.numSamples=256;
 EventList noOutputEvents(4);
 Event noOutputNote{};
 noOutputNote.type=Event::kNoteOnEvent;
 noOutputNote.sampleOffset=10;
 noOutputNote.noteOn.pitch=27; // pattern D
 noOutputNote.noteOn.velocity=1.f;
 require(noOutputEvents.addEvent(noOutputNote)==kResultOk,"F05 MIDI event");
 flushData.inputEvents=&noOutputEvents;
 flushData.inputParameterChanges=nullptr;
 require(audio->process(flushData)==kResultOk,"F05 no-output MIDI process");
 Steinberg::ResizableMemoryIBStream f05State;
 require(component->getState(&f05State)==kResultOk,"F05 state save");
 drumator::KitState f05Kit;
 require(drumator::KitCodec::decode(
   static_cast<const std::uint8_t*>(f05State.getData()),
   f05State.getCursor(),f05Kit)&&f05Kit.selected==3,
   "F05 MIDI selection discarded without output buses");
 ParameterChanges f05Automation;
 int32 f05QueueIndex=0;
 auto* f05Queue=f05Automation.addParameterData(drumator::kPatternParam,f05QueueIndex);
 require(f05Queue!=nullptr,"F05 automation queue");
 int32 f05Index=0;
 require(f05Queue->addPoint(10,6.0/7.0,f05Index)==kResultOk,
   "F05 simultaneous host point");
 flushData.inputParameterChanges=&f05Automation;
 require(audio->process(flushData)==kResultOk,"F05 host MIDI tie");
 Steinberg::ResizableMemoryIBStream f05TieState;
 require(component->getState(&f05TieState)==kResultOk,"F05 tied state save");
 require(drumator::KitCodec::decode(
   static_cast<const std::uint8_t*>(f05TieState.getData()),
   f05TieState.getCursor(),f05Kit)&&f05Kit.selected==6,
   "F05 host did not win same-offset MIDI event");
 // F05b: two MIDI pattern changes in an outputless block must use
 // sample order, not event insertion order or the previous selected pattern.
 {
   EventList orderedMidi(4);
   Event early{};
   early.type=Event::kNoteOnEvent;
   early.sampleOffset=20;
   early.noteOn.pitch=25; // B
   early.noteOn.velocity=1.f;
   Event late=early;
   late.sampleOffset=150;
   late.noteOn.pitch=30; // G
   // Deliberately submit the later event first: the timeline must sort by
   // sample offset, not rely on the host's event enumeration order.
   require(orderedMidi.addEvent(late)==kResultOk,"F05b late MIDI");
   require(orderedMidi.addEvent(early)==kResultOk,"F05b early MIDI");
   flushData.inputEvents=&orderedMidi;
   flushData.inputParameterChanges=nullptr;
   require(audio->process(flushData)==kResultOk,"F05b outputless ordered MIDI");
   Steinberg::ResizableMemoryIBStream orderedState;
   require(component->getState(&orderedState)==kResultOk,"F05b state save");
   drumator::KitState orderedKit;
   require(drumator::KitCodec::decode(
       static_cast<const std::uint8_t*>(orderedState.getData()),
       orderedState.getCursor(),orderedKit)&&orderedKit.selected==6,
       "F05b later MIDI pattern did not win");
   flushData.inputEvents=&noOutputEvents;
 }
 std::cout<<"F05b outputless MIDI sample order PASS\\n";
 // T30: more than 256 MIDI program commands in one real Processor block.
 // Previously a 256-entry collection overflow could discard the entire
 // set. The final host point must win the MIDI tie at the final frame.
 {
   EventList denseMidi(320);
   for(int n=0;n<300;++n) {
     Event e{};
     e.type=Event::kNoteOnEvent;
     e.sampleOffset=std::min(n,255);
     e.noteOn.pitch=static_cast<Steinberg::int16>(24+n%8);
     e.noteOn.velocity=1.f;
     require(denseMidi.addEvent(e)==kResultOk,"T30 dense MIDI creation");
   }
   ParameterChanges denseHost;
   int32 denseQueueIndex=0;
   auto* queue=denseHost.addParameterData(drumator::kPatternParam,denseQueueIndex);
   require(queue!=nullptr,"T30 dense host queue");
   int32 densePointIndex=0;
   require(queue->addPoint(255,6.0/7.0,densePointIndex)==kResultOk,
           "T30 host tie creation");
   // A final host-only G would also pass if all 300 MIDI commands vanished.
   // Require MIDI-only D first and real feedback, on BOTH process branches.
   for(bool rendered:{false,true})for(bool hostTie:{false,true}) {
     auto denseData=rendered?data:flushData;
     ParameterChanges feedback;
     denseData.inputEvents=&denseMidi;
     denseData.inputParameterChanges=hostTie?&denseHost:nullptr;
     denseData.outputParameterChanges=&feedback;
     require(audio->process(denseData)==kResultOk,"T30 dense processor events");
     Steinberg::ResizableMemoryIBStream denseState;
     require(component->getState(&denseState)==kResultOk,"T30 state save");
     drumator::KitState denseKit;
     require(drumator::KitCodec::decode(
         static_cast<const std::uint8_t*>(denseState.getData()),
         denseState.getCursor(),denseKit)&&denseKit.selected==(hostTie?6:3),
         "T30 dense host/MIDI arbitration lost final selection");
     require(feedback.getParameterCount()==1,"T30 MIDI was not reported to host");
     auto* reported=feedback.getParameterData(0);
     int32 offset=-1;ParamValue value=-1.;
     require(reported&&reported->getParameterId()==drumator::kPatternParam&&
             reported->getPointCount()==(hostTie?255:256)&&
             reported->getPoint(reported->getPointCount()-1,offset,value)==kResultOk&&
             offset==(hostTie?254:255)&&
             drumator::patternFromNormalized(value)==(hostTie?6u:3u),
             "T30 chronological feedback lost events, accepted a MIDI host tie, or reordered the overflow tail");
     for(int32 point=0;point<reported->getPointCount();++point)
       require(reported->getPoint(point,offset,value)==kResultOk&&offset==point&&
               drumator::patternFromNormalized(value)==(point==255?3u:static_cast<unsigned>(point%8)),
               "T30 intermediate MIDI feedback differs from independent event sequence");
   }
   drumator::qa::result("T30 PASS: 300 MIDI commands, MIDI-only D and host-tie G, rendered/outputless paths and all 255/256 feedback offsets");
 }
 std::cout<<"T30 >256 real MIDI events and final host tie PASS\\n";

 // T18: Song mode suppresses MIDI pattern selection even when there
 // are no output buses; the outputless path must not reject the block.
 {
   HostMessage song;
   song.setMessageID("Drumator.SongConfig");
   auto* a=song.getAttributes();
   const std::array<std::uint8_t,2> entry{6,16};
   require(a&&a->setInt("Enabled",1)==kResultOk&&
       a->setInt("Count",1)==kResultOk&&
       a->setBinary("Entries",entry.data(),2)==kResultOk&&
       processorPoint->notify(&song)==kResultOk,"T18 enable Song mode");
 }
 flushData.inputParameterChanges=nullptr;
 // Song state must also be adopted on outputless blocks with no MIDI.
 // Follow with a MIDI-bearing block to exercise the cached snapshot.
 flushData.inputEvents=nullptr;
 require(audio->process(flushData)==kResultOk,"T24 song-only outputless state update");
 flushData.inputEvents=&noOutputEvents;
 require(audio->process(flushData)==kResultOk,"T18 outputless song processing");
 Steinberg::ResizableMemoryIBStream t18Saved;
 require(component->getState(&t18Saved)==kResultOk,"T18 state save");
 drumator::KitState t18Kit;
 require(drumator::KitCodec::decode(
   static_cast<const std::uint8_t*>(t18Saved.getData()),
   t18Saved.getCursor(),t18Kit)&&t18Kit.selected==6&&t18Kit.songEnabled==1,
   "T18 MIDI overrode song-mode selection");
 {
   HostMessage song;
   song.setMessageID("Drumator.SongConfig");
   auto* a=song.getAttributes();
   const std::array<std::uint8_t,2> entry{6,16};
   require(a&&a->setInt("Enabled",0)==kResultOk&&
       a->setInt("Count",1)==kResultOk&&
       a->setBinary("Entries",entry.data(),2)==kResultOk&&
       processorPoint->notify(&song)==kResultOk,"T18 restore pattern mode");
 }
 // T27: disable Song mode in an outputless block with no MIDI,
 // then verify the following MIDI-only block once again selects A-H.
 flushData.inputEvents=nullptr;
 flushData.inputParameterChanges=nullptr;
 require(audio->process(flushData)==kResultOk,
         "T27 outputless Song disable without MIDI");
 flushData.inputEvents=&noOutputEvents; // D (pitch 27)
 require(audio->process(flushData)==kResultOk,
         "T27 MIDI selection after Song disable");
 Steinberg::ResizableMemoryIBStream t27Saved;
 require(component->getState(&t27Saved)==kResultOk,"T27 state save");
 drumator::KitState t27Kit;
 require(drumator::KitCodec::decode(
   static_cast<const std::uint8_t*>(t27Saved.getData()),
   t27Saved.getCursor(),t27Kit)&&t27Kit.selected==3&&t27Kit.songEnabled==0,
   "T27 MIDI selection remained suppressed after Song disabled");
 flushData.inputEvents=nullptr;
 flushData.inputParameterChanges=&flushChanges;
 flushData.numSamples=0;
 std::cout<<"T27 outputless Song disable restores MIDI selection PASS\\n";
 std::cout<<"F05 outputless MIDI A-H and host tie arbitration PASS\\n";
 std::cout<<"T18 outputless Song mode MIDI suppression PASS\\n";
 // F08 integration: deactivate MAIN while a real auxiliary row stays active.
 // Bus 1 is row 1 in the 125A fixed-row routing contract.
 require(component->activateBus(kAudio,kOutput,0,false)==kResultOk,
         "F08 disable MAIN");
 require(component->activateBus(kAudio,kOutput,1,true)==kResultOk,
         "F08 activate auxiliary bus");
 std::array<float,256> f08StemLeft{},f08StemRight{};
 float* stemSamples[2]{f08StemLeft.data(),f08StemRight.data()};
 std::array<AudioBusBuffers,2> stemBuses{};
 stemBuses[0].numChannels=2;stemBuses[0].channelBuffers32=nullptr;
 stemBuses[1].numChannels=2;stemBuses[1].channelBuffers32=stemSamples;
 const auto priorOutputs=data.outputs;
 const auto priorNumOutputs=data.numOutputs;
 data.outputs=stemBuses.data();data.numOutputs=2;
 context.state=0;
 events.clear();
 Event stemEvent{};stemEvent.type=Event::kNoteOnEvent;
 stemEvent.sampleOffset=0;stemEvent.noteOn.pitch=37;
 stemEvent.noteOn.velocity=1.f;stemEvent.noteOn.noteId=-1;
 require(events.addEvent(stemEvent)==kResultOk,"F08 add row 1 MIDI trigger");
 require(audio->process(data)==kResultOk,"F08 process with no MAIN buffers");
 // T29 uses isolated real kit state and fresh voices, with independent L/R,
 // pre-trigger silence and both null/non-null zero-channel MAIN tables.
 auditZeroChannelMain(component,audio,setup.sampleRate);
 for(float v:f08StemLeft)require(std::isfinite(v),"F08 invalid stem L");
 for(float v:f08StemRight)require(std::isfinite(v),"F08 invalid stem R");
 data.outputs=priorOutputs;data.numOutputs=priorNumOutputs;
 require(component->activateBus(kAudio,kOutput,0,true)==kResultOk,
         "F08 reenable MAIN");
 std::cout<<"F08 packaged auxiliary processing without MAIN PASS\\n";
 // F08 full packaged 16-bus matrix: fifteen simultaneous row sounds must
 // reach their individual AUX1..AUX15 despite a NULL MAIN buffer.
 for(int i=0;i<12;++i)run(-1);
 for(int row=0;row<16;++row) {
   for(int solo=0;solo<=1;++solo) {
     HostMessage gate;gate.setMessageID("Drumator.RowGate");
     auto* attr=gate.getAttributes();
     require(attr->setInt("Row",row)==kResultOk&&
             attr->setInt("Solo",solo)==kResultOk&&
             attr->setInt("On",0)==kResultOk,
             "F08 reset row gate attrs");
     require(processorPoint->notify(&gate)==kResultOk,
             "F08 reset row gates");
   }
   HostMessage choke;choke.setMessageID("Drumator.RowChoke");
   auto* ca=choke.getAttributes();
   require(ca->setInt("Row",row)==kResultOk&&
           ca->setInt("Group",0)==kResultOk&&
           processorPoint->notify(&choke)==kResultOk,
           "F08 reset row choke");
 }
 std::array<float,16> expectedBus{},expectedRight{};
 for(int busIndex=1;busIndex<16;++busIndex) {
   const float stemAmplitude=0.04f+static_cast<float>(busIndex)*0.015f;
   expectedBus[static_cast<std::size_t>(busIndex)]=stemAmplitude;
   expectedRight[static_cast<std::size_t>(busIndex)]=-0.03f-static_cast<float>(busIndex)*0.011f;
   require(send(makeStereoWav(stemAmplitude,expectedRight[static_cast<std::size_t>(busIndex)]),busIndex)==kResultOk,
           "F08 load dedicated AUX test WAV");
   HostMessage mix;mix.setMessageID("Drumator.RowMix");
   auto* m=mix.getAttributes();
   require(m->setInt("Row",busIndex)==kResultOk&&
           m->setInt("Gain",100)==kResultOk&&
           m->setInt("Pan",0)==kResultOk&&
           processorPoint->notify(&mix)==kResultOk,
           "F08 normalize AUX row gain and pan");
   require(component->activateBus(kAudio,kOutput,busIndex,true)==kResultOk,
           "F08 activate AUX output");
 }
 require(component->activateBus(kAudio,kOutput,0,false)==kResultOk,
         "F08 disable MAIN for full matrix");
 std::array<std::array<float,256>,16> matrixLeft{},matrixRight{};
 std::array<std::array<float*,2>,16> matrixPointers{};
 std::array<AudioBusBuffers,16> matrixBuses{};
 matrixBuses[0].numChannels=2;matrixBuses[0].channelBuffers32=nullptr;
 for(int busIndex=1;busIndex<16;++busIndex) {
   matrixPointers[static_cast<std::size_t>(busIndex)]={{
      matrixLeft[static_cast<std::size_t>(busIndex)].data(),
      matrixRight[static_cast<std::size_t>(busIndex)].data()}};
   matrixBuses[static_cast<std::size_t>(busIndex)].numChannels=2;
   matrixBuses[static_cast<std::size_t>(busIndex)].channelBuffers32=
       matrixPointers[static_cast<std::size_t>(busIndex)].data();
 }
 EventList simultaneousNotes(24);
 for(int row=1;row<16;++row) {
   Event hit{};hit.type=Event::kNoteOnEvent;hit.sampleOffset=0;
   hit.noteOn.pitch=static_cast<int16>(36+row);
   hit.noteOn.velocity=1.f;hit.noteOn.noteId=-1;
   require(simultaneousNotes.addEvent(hit)==kResultOk,
           "F08 append simultaneous row MIDI");
 }
 auto* oldInputEvents=data.inputEvents;
 data.inputEvents=&simultaneousNotes;
 data.outputs=matrixBuses.data();data.numOutputs=16;
 require(audio->process(data)==kResultOk,
         "F08 sixteen-output processing with null MAIN");
 std::array<double,16> measuredL{},measuredR{};
 for(int busIndex=1;busIndex<16;++busIndex) {
   double avgL=0.,avgR=0.;
   for(int i=80;i<180;++i) {
     avgL+=matrixLeft[static_cast<std::size_t>(busIndex)][static_cast<std::size_t>(i)];
     avgR+=matrixRight[static_cast<std::size_t>(busIndex)][static_cast<std::size_t>(i)];
   }
   avgL/=100.;avgR/=100.;
   measuredL[static_cast<std::size_t>(busIndex)]=avgL;
   measuredR[static_cast<std::size_t>(busIndex)]=avgR;
 }
 auto routingMatches=[&](const auto& l,const auto& r) {
   // Constant float32 plateau at unity gain: 1e-4 allows rounding/filter error,
   // yet stays over 100 times below the smallest inter-bus/channel code gap.
   for(std::size_t b=1;b<16;++b)
     if(!std::isfinite(l[b])||!std::isfinite(r[b])||
        std::abs(l[b]-expectedBus[b])>1e-4||
        std::abs(r[b]-expectedRight[b])>1e-4)return false;
   return true;
 };
 require(routingMatches(measuredL,measuredR),"T02 AUX/stereo signature mismatch");
 auto swappedL=measuredL,swappedR=measuredR;
 std::swap(swappedL[7],swappedL[8]);std::swap(swappedR[7],swappedR[8]);
 require(!routingMatches(swappedL,swappedR),"T02 accepted swapped adjacent AUX buses");
 std::swap(swappedL[7],swappedL[8]);std::swap(swappedR[7],swappedR[8]);
 std::swap(swappedL[5],swappedR[5]);
 require(!routingMatches(swappedL,swappedR),"T02 accepted reversed stereo channels");
 drumator::qa::result("T02 PASS: 15 distinct AUX stereo codes, neighbor-bus and L/R swap negatives");
 std::cout<<"T02 15 unique stereo stems; adjacent-bus and L/R negatives PASS\n";
 data.inputEvents=oldInputEvents;
 data.outputs=priorOutputs;data.numOutputs=priorNumOutputs;
 require(component->activateBus(kAudio,kOutput,0,true)==kResultOk,
         "F08 restore MAIN after full matrix");
 std::cout<<"F08 fifteen simultaneous AUX stems, no MAIN, measured PASS\\n";

 // T04: two genuinely different kits, coherent four-channel audio signatures.
 Steinberg::ResizableMemoryIBStream runningKitState;
 require(component->getState(&runningKitState)==kResultOk,"T04 save baseline");
 drumator::KitState kitX,kitY;
 kitX.grid.fill(0);kitX.visible=2;kitX.selected=0;
 kitX.wav[0]=makeStereoWav(.25f,-.35f);kitX.names[0]="X0.wav";
 kitX.wav[1]=makeStereoWav(-.10f,.05f);kitX.names[1]="X1.wav";
 kitX.grid[0]=127;kitX.grid[drumator::kSteps]=64;
 kitX.gain[1]=50;kitX.masterVolume=100;
 kitY=kitX;kitY.selected=1;kitY.grid.fill(0);
 kitY.grid[drumator::kRows*drumator::kSteps]=64;
 kitY.grid[drumator::kRows*drumator::kSteps+drumator::kSteps]=127;
 kitY.wav[0]=makeStereoWav(-.30f,.20f);kitY.names[0]="Y0.wav";
 kitY.wav[1]=makeStereoWav(.07f,-.11f);kitY.names[1]="Y1.wav";
 kitY.gain[0]=50;kitY.gain[1]=150;kitY.masterVolume=70;
 kitY.output[0]=1;kitY.output[1]=0;
 std::vector<std::uint8_t> bytesX,bytesY;
 require(drumator::KitCodec::encode(kitX,bytesX)&&drumator::KitCodec::encode(kitY,bytesY),"T04 encode kits");
 auto restoreBytes=[&](const auto& bytes) {
   drumator::qa::ByteStream stream(const_cast<std::uint8_t*>(bytes.data()),bytes.size());
   return component->setState(&stream);
 };
 require(component->activateBus(kAudio,kOutput,1,true)==kResultOk,"T04 enable second bus");
 require(restoreBytes(bytesX)==kResultOk,"T04 initial X");
 const double v64=64./127.;
 const std::array<double,4> codeX{{.25,-.35,-.10*.5*v64,.05*.5*v64}};
 const std::array<double,4> codeY{{.07*1.5*.7,-.11*1.5*.7,-.30*.5*v64*.7,.20*.5*v64*.7}};
 auto isCode=[](const auto& actual,const auto& expected) {
   for(std::size_t c=0;c<4;++c)
     if(!std::isfinite(actual[c])||std::abs(actual[c]-expected[c])>2e-4)return false;
   return true;
 };
 auto hybrid=codeX;hybrid[2]=codeY[2];
 require(!isCode(hybrid,codeX)&&!isCode(hybrid,codeY),"T04 mixed-generation negative escaped");
 std::atomic<bool> stopAudio{false},audioFailed{false};
 std::atomic<unsigned> audioBlocks{0},xBlocks{0},yBlocks{0},gateSilent{0};
 std::atomic<unsigned> wantedEpoch{0},acknowledgedEpoch{0};
 std::thread audioActor([&](){
   std::array<std::array<float,256>,4> buffers{};
   float* p0[]{buffers[0].data(),buffers[1].data()};
   float* p1[]{buffers[2].data(),buffers[3].data()};
   AudioBusBuffers buses[2]{};
   buses[0].numChannels=2;buses[0].channelBuffers32=p0;
   buses[1].numChannels=2;buses[1].channelBuffers32=p1;
   ProcessContext clock{};clock.tempo=120;clock.projectTimeMusic=0;
   clock.state=ProcessContext::kPlaying|ProcessContext::kProjectTimeMusicValid|ProcessContext::kTempoValid;
   ProcessData rt{};rt.numSamples=256;rt.numOutputs=2;rt.outputs=buses;
   rt.symbolicSampleSize=kSample32;rt.processMode=kRealtime;rt.processContext=&clock;
   while(!stopAudio.load(std::memory_order_acquire)) {
     const auto epoch=wantedEpoch.load(std::memory_order_acquire);
     // Stop clears the previous one-shot voices. Merely locating to PPQ zero
     // does not stop them; repeated zero-position blocks would stack voices
     // and turn a correct engine into a false mixed-generation failure.
     clock.state=0;
     if(audio->process(rt)!=kResultOk){audioFailed=true;break;}
     clock.state=ProcessContext::kPlaying|ProcessContext::kProjectTimeMusicValid|ProcessContext::kTempoValid;
     if(audio->process(rt)!=kResultOk){audioFailed=true;break;}
     std::array<double,4> code{};
     bool silent=true;
     for(std::size_t c=0;c<4;++c)for(std::size_t n=80;n<180;++n) {
       const auto sample=buffers[c][n];
       if(!std::isfinite(sample))audioFailed=true;
       code[c]+=sample/100.;if(std::abs(sample)>1e-7)silent=false;
     }
     const bool x=isCode(code,codeX),y=isCode(code,codeY);
     if(silent)++gateSilent;
     else if(x)++xBlocks;
     else if(y)++yBlocks;
     else {audioFailed=true;break;}
     ++audioBlocks;
     if(epoch&&((epoch&1u)?y:x))acknowledgedEpoch.store(epoch,std::memory_order_release);
     if(audioFailed.load())break;
     std::this_thread::yield();
   }
 });
 auto waitFor=[&](auto predicate) {
   const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);
   while(!predicate()&&!audioFailed.load()&&std::chrono::steady_clock::now()<until)std::this_thread::yield();
   return predicate()&&!audioFailed.load();
 };
 require(waitFor([&]{return xBlocks.load()>0;}),"T04 initial rendered X absent");
 for(unsigned epoch=1;epoch<=24;++epoch) {
   require(restoreBytes((epoch&1u)?bytesY:bytesX)==kResultOk,"T04 concurrent restore rejected");
   wantedEpoch.store(epoch,std::memory_order_release);
   if(!waitFor([&]{return acknowledgedEpoch.load(std::memory_order_acquire)==epoch;})) {
     stopAudio=true;audioActor.join();require(false,"T04 new coherent kit audio absent or mixed");
   }
   Steinberg::ResizableMemoryIBStream capturedKitState;
   drumator::KitState observed;
   const auto& expected=(epoch&1u)?kitY:kitX;
   require(component->getState(&capturedKitState)==kResultOk&&
       drumator::KitCodec::decode(static_cast<const std::uint8_t*>(capturedKitState.getData()),capturedKitState.getCursor(),observed)&&
       observed.grid==expected.grid&&observed.wav==expected.wav&&observed.names==expected.names&&
       observed.gain==expected.gain&&observed.output==expected.output&&
       observed.masterVolume==expected.masterVolume&&observed.selected==expected.selected,
       "T04 serialized kit mixed generations");
 }
 stopAudio=true;audioActor.join();
 require(!audioFailed.load()&&xBlocks.load()>0&&yBlocks.load()>0,"T04 incomplete concurrency coverage");
 drumator::qa::result("T04 PASS: 24 X/Y restores; coherent X blocks="+std::to_string(xBlocks.load())+
   ", Y="+std::to_string(yBlocks.load())+", gate silent="+std::to_string(gateSilent.load()));
 std::cout<<"T04 actual packaged audio: X="<<xBlocks<<" Y="<<yBlocks<<" gate-silent="<<gateSilent
          <<" total="<<audioBlocks<<"; 24 acknowledged restores PASS\n";
 // T06: each RowMix message is one logical Gain+Pan transaction.
 // Concurrent control-side project exports must never save hybrid pairs.
 {
   auto setPair=[&](int gain,int pan) {
     HostMessage message;message.setMessageID("Drumator.RowMix");
     auto* attr=message.getAttributes();
     if(!attr||attr->setInt("Row",0)!=kResultOk||
        attr->setInt("Gain",gain)!=kResultOk||
        attr->setInt("Pan",pan)!=kResultOk)return false;
     return processorPoint->notify(&message)==kResultOk;
   };
   require(setPair(42,-65),"T06 setup first gain/pan pair");
   std::atomic<bool> completed{false},failed{false};
   std::thread writer([&] {
     for(int i=0;i<400;++i){
       if(!setPair(i&1?42:180,i&1?-65:75)){
         failed.store(true,std::memory_order_release);
         break;
       }
     }
     completed.store(true,std::memory_order_release);
   });
   unsigned exported=0;
   do {
     Steinberg::ResizableMemoryIBStream stream;
     drumator::KitState state;
     if(component->getState(&stream)!=kResultOk||
        !drumator::KitCodec::decode(
             static_cast<const std::uint8_t*>(stream.getData()),
             static_cast<std::size_t>(stream.getCursor()),state)){
       failed.store(true,std::memory_order_release);
       break;
     }
     const bool A=state.gain[0]==42&&state.pan[0]==35;
     const bool B=state.gain[0]==180&&state.pan[0]==175;
     if(!A&&!B)failed.store(true,std::memory_order_release);
     ++exported;
   }while(!completed.load(std::memory_order_acquire)&&!failed.load(std::memory_order_acquire));
   writer.join();
   require(!failed.load()&&exported>0,"T06 torn RowMix gain/pan project snapshot");
   std::cout<<"T06 concurrent RowMix paired-state export PASS\\n";
 }
 runningKitState.rewind();
 require(component->setState(&runningKitState)==kResultOk,"T04 restore original fixture");
 // F07: reproduce a valid portable kit containing a hidden solo row.
 // Its samples stay intact, but invisible solo flags must not mute row 1.
 Steinberg::ResizableMemoryIBStream baselineKit;
 require(component->getState(&baselineKit)==kResultOk,"F07 baseline snapshot");
 drumator::KitState hiddenSoloKit;
 require(drumator::KitCodec::decode(
         static_cast<const std::uint8_t*>(baselineKit.getData()),
         baselineKit.getCursor(),hiddenSoloKit),"F07 baseline decode");
 hiddenSoloKit.visible=4;
 hiddenSoloKit.solo=static_cast<std::uint16_t>(1u<<7);
 std::vector<std::uint8_t> hiddenBytes;
 require(drumator::KitCodec::encode(hiddenSoloKit,hiddenBytes),
         "F07 hidden solo fixture encode");
 Steinberg::ResizableMemoryIBStream hiddenState;
 Steinberg::int32 hiddenWritten=0;
 require(hiddenState.write(hiddenBytes.data(),
           static_cast<Steinberg::int32>(hiddenBytes.size()),
           &hiddenWritten)==kResultOk&&
           static_cast<std::size_t>(hiddenWritten)==hiddenBytes.size(),
           "F07 hidden solo fixture stream");
 hiddenState.rewind();
 require(component->setState(&hiddenState)==kResultOk,
         "F07 processor hidden solo recall");
 hiddenState.rewind();
 require(ctrl->setComponentState(&hiddenState)==kResultOk,
         "F07 controller hidden solo recall");
 Steinberg::ResizableMemoryIBStream sanitized;
 require(component->getState(&sanitized)==kResultOk,"F07 sanitized state");
 drumator::KitState sanitizedKit;
 require(drumator::KitCodec::decode(
           static_cast<const std::uint8_t*>(sanitized.getData()),
           sanitized.getCursor(),sanitizedKit)&&
         sanitizedKit.solo==0&&sanitizedKit.visible==4,
         "F07 hidden solo stayed active after state load");
 baselineKit.rewind();
 require(component->setState(&baselineKit)==kResultOk,
         "F07 restore baseline kit");
 std::cout<<"F07 packaged VST3 hidden-solo recall policy PASS\\n";
 // N08: after restoring a long drum hit, a single playing block followed
 // immediately by STOP must clear all tails in the next host block.
 drumator::KitState stopKit;
 require(drumator::KitCodec::decode(
           static_cast<const std::uint8_t*>(baselineKit.getData()),
           baselineKit.getCursor(),stopKit),
         "N08 baseline decode");
 stopKit.grid.fill(0);
 stopKit.grid[0]=127;
 stopKit.selected=0;
 stopKit.solo=0;stopKit.muted=0;
 stopKit.swing=0;stopKit.humanize=0;
 stopKit.wav[0]=makeWav(32,true,.25f);
 stopKit.names[0]="N08_Transport.wav";
 std::vector<std::uint8_t> stopBytes;
 require(drumator::KitCodec::encode(stopKit,stopBytes),
         "N08 fixture encode");
 Steinberg::ResizableMemoryIBStream stopState;
 Steinberg::int32 stopWritten=0;
 require(stopState.write(stopBytes.data(),
         static_cast<Steinberg::int32>(stopBytes.size()),
         &stopWritten)==kResultOk&&
         static_cast<std::size_t>(stopWritten)==stopBytes.size(),
         "N08 fixture stream");
 stopState.rewind();
 require(component->setState(&stopState)==kResultOk,"N08 recall");
 auto* previousChanges=data.inputParameterChanges;
 data.inputParameterChanges=nullptr;
 context.state=ProcessContext::kPlaying|
               ProcessContext::kProjectTimeMusicValid|
               ProcessContext::kTempoValid;
 context.projectTimeMusic=0.;
 context.tempo=120.;
 run(-1);
 double playingEnergy=0.;
 for(float v:left)playingEnergy+=std::abs(v);
 require(playingEnergy>1.,"N08 fixture did not start a sequencer voice");
 context.state=0;
 run(-1);
 for(int i=0;i<256;++i)
   require(left[i]==0.f&&right[i]==0.f,
           "N08 immediate stop failed to silence long sample tail");
 data.inputParameterChanges=previousChanges;
 std::cout<<"N08 kit-recall playing-block immediate-stop regression PASS\\n";
 require(audio->setProcessing(false)==kResultOk,"Stop processing");
 require(component->setActive(false)==kResultOk,"Deactivate final");
 require(controlPoint->disconnect(processorPoint)==kResultOk,"Disconnect controller");
 require(processorPoint->disconnect(controlPoint)==kResultOk,"Disconnect processor");
 require(ctrl->terminate()==kResultOk,"Controller terminate");
 require(component->terminate()==kResultOk,"Processor terminate");
 controlPoint->release();processorPoint->release();audio->release();
 ctrl->release();component->release();factory->release();
 if(auto* exitDll=reinterpret_cast<PluginInit>(GetProcAddress(dll,"ExitDll")))
   require(exitDll(),"ExitDll");
 FreeLibrary(dll);
 drumator::qa::result("125A Drumator packaged audio-host smoke PASS: all preceding require checks completed");
}
