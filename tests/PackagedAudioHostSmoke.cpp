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

using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {
void require(bool value,const char* message) {
 if(!value) {std::cerr<<"Packaged Drumator host FAIL: "<<message<<'\n';std::exit(2);}
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
         controllerZoom.getCursor()==5,"F15 initial zoom preference state");
 std::array<std::uint8_t,5> zoom150State{{0x44,0x5a,0x4d,0x31,1}};
 Steinberg::ResizableMemoryIBStream zoomRecall;
 Steinberg::int32 zoomBytes=0;
 require(zoomRecall.write(zoom150State.data(),5,&zoomBytes)==kResultOk&&
         zoomBytes==5,"F15 write controller zoom fixture");
 zoomRecall.rewind();
 require(ctrl->setState(&zoomRecall)==kResultOk,"F15 restore 150 percent zoom");
 Steinberg::ResizableMemoryIBStream zoomSaved;
 require(ctrl->getState(&zoomSaved)==kResultOk&&zoomSaved.getCursor()==5&&
         std::memcmp(zoomSaved.getData(),zoom150State.data(),5)==0,
         "F15 zoom preference failed roundtrip");
 std::cout<<"F15 controller zoom preference project-state PASS\\n";
 require(controlPoint->connect(processorPoint)==kResultOk,"Controller connection");
 require(processorPoint->connect(controlPoint)==kResultOk,"Processor connection");
 SpeakerArrangement stereo=SpeakerArr::kStereo;
 require(audio->setBusArrangements(nullptr,0,&stereo,1)==kResultOk,"Stereo bus");
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
 Steinberg::ResizableMemoryIBStream saved;
 require(component->getState(&saved)==kResultOk,"VST3 getState failed");
 const auto* raw=static_cast<const std::uint8_t*>(saved.getData());
 drumator::KitState savedKit;
 require(drumator::KitCodec::decode(raw,saved.getCursor(),savedKit),
         "Complete DMR4 kit not saved");
 require(savedKit.wav[0]==makeWav(32,true,-.5f),
         "Original FLOAT32 WAV was not embedded in song state");
 require(savedKit.names[0]=="StudioDrum.wav","WAV filename missing from song");
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
 require(restoredAudio->setBusArrangements(nullptr,0,&stereo,1)==kResultOk,
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
 require(kitAudio->setBusArrangements(nullptr,0,&stereo,1)==kResultOk,
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
 // M27: activate stereo stem for row 2 and verify output isolation.
 require(component->activateBus(kAudio,kOutput,1,true)==kResultOk,
         "Activate row 2 stereo stem");
 std::array<float,256> stemLeft{},stemRight{};
 float* stemChannels[2]{stemLeft.data(),stemRight.data()};
 AudioBusBuffers stemBus{};
 stemBus.numChannels=2;stemBus.channelBuffers32=stemChannels;
 AudioBusBuffers routedBuses[2]{bus,stemBus};
 data.numOutputs=2;data.outputs=routedBuses;
 for(int n=0;n<6;++n)run(-1);
 run(37);
 double mainSum=0,stemSum=0;
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
 require(audio->setupProcessing(setup)==kResultOk,
         "Host sample rate 48k to 44.1k setup");
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
 std::array<float,16> expectedBus{};
 for(int busIndex=1;busIndex<16;++busIndex) {
   const float stemAmplitude=0.04f+static_cast<float>(busIndex)*0.015f;
   expectedBus[static_cast<std::size_t>(busIndex)]=stemAmplitude;
   require(send(makeWav(32,true,stemAmplitude),busIndex)==kResultOk,
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
 for(int busIndex=1;busIndex<16;++busIndex) {
   double avgL=0.,avgR=0.;
   for(int i=80;i<180;++i) {
     avgL+=matrixLeft[static_cast<std::size_t>(busIndex)][static_cast<std::size_t>(i)];
     avgR+=matrixRight[static_cast<std::size_t>(busIndex)][static_cast<std::size_t>(i)];
   }
   avgL/=100.;avgR/=100.;
   const double expected=expectedBus[static_cast<std::size_t>(busIndex)];
   require(std::isfinite(avgL)&&std::isfinite(avgR)&&
           std::abs(avgL-expected)<.03&&std::abs(avgR-expected)<.03,
           "F08 15-AUX routing amplitude/isolation mismatch");
 }
 data.inputEvents=oldInputEvents;
 data.outputs=priorOutputs;data.numOutputs=priorNumOutputs;
 require(component->activateBus(kAudio,kOutput,0,true)==kResultOk,
         "F08 restore MAIN after full matrix");
 std::cout<<"F08 fifteen simultaneous AUX stems, no MAIN, measured PASS\\n";

 // P1 kit-recall interleaving regression: VST3 setState remains supported
 // while a separate realtime actor repeatedly invokes process().
 Steinberg::ResizableMemoryIBStream runningKitState;
 require(component->getState(&runningKitState)==kResultOk,
         "Concurrent state fixture save");
 std::atomic<bool> stopAudio{false},audioFailed{false};
 std::atomic<unsigned> audioBlocks{0};
 std::thread audioActor([&](){
   std::array<float,256> rtLeft{},rtRight{};
   float* rtPtrs[2]{rtLeft.data(),rtRight.data()};
   AudioBusBuffers rtBus{};
   rtBus.numChannels=2;rtBus.channelBuffers32=rtPtrs;
   EventList rtEvents(2);
   Event hit{};hit.type=Event::kNoteOnEvent;
   hit.noteOn.pitch=36;hit.noteOn.velocity=1.f;hit.noteOn.noteId=-1;
   rtEvents.addEvent(hit);
   ProcessContext rtContext{};rtContext.state=0;
   ProcessData rtData{};
   rtData.numSamples=256;rtData.numOutputs=1;rtData.outputs=&rtBus;
   rtData.symbolicSampleSize=kSample32;rtData.processMode=kRealtime;
   rtData.inputEvents=&rtEvents;rtData.processContext=&rtContext;
   while(!stopAudio.load(std::memory_order_acquire)&&
         audioBlocks.load(std::memory_order_relaxed)<4000) {
     if(audio->process(rtData)!=kResultOk) {
       audioFailed.store(true);break;
     }
     for(int n=0;n<256;++n) {
       if(!std::isfinite(rtLeft[n])||!std::isfinite(rtRight[n])) {
         audioFailed.store(true);break;
       }
     }
     audioBlocks.fetch_add(1,std::memory_order_relaxed);
     if(audioFailed.load())break;
   }
 });
 for(int n=0;n<6;++n) {
   runningKitState.rewind();
   require(component->setState(&runningKitState)==kResultOk,
           "P1 concurrent complete kit restore failed");
   std::this_thread::yield();
 }
 stopAudio.store(true,std::memory_order_release);
 audioActor.join();
 require(!audioFailed.load()&&audioBlocks.load()>0,
         "P1 concurrent kit processing returned invalid data");
 std::cout<<"P1 concurrent complete kit recall and live audio PASS\\n";
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
 std::cout<<"125A Drumator packaged audio-host smoke PASS\n";
}
