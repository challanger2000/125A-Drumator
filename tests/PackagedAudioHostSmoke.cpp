// Actual packaged Windows VST3 instantiation + MIDI and PCM audio smoke.
// Deliberately no Studio One proprietary code. Fails if the plugin compiles
// but doesn't render a real decoded sample on its real output bus.
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "vst3/PluginIDs.h"
#include "core/KitState.h"
#include "core/FlacImport.h"
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
 auto* dll=LoadLibraryW(std::filesystem::u8path(argv[1]).c_str());
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
 require(controlPoint->connect(processorPoint)==kResultOk,"Controller connection");
 require(processorPoint->connect(controlPoint)==kResultOk,"Processor connection");
 SpeakerArrangement stereo=SpeakerArr::kStereo;
 require(audio->setBusArrangements(nullptr,0,&stereo,1)==kResultOk,"Stereo bus");
 ProcessSetup setup{};
 setup.sampleRate=48000;setup.maxSamplesPerBlock=256;
 setup.symbolicSampleSize=kSample32;setup.processMode=kRealtime;
 require(audio->setupProcessing(setup)==kResultOk,"setupProcessing 48 kHz");
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
 Steinberg::ResizableMemoryIBStream saved;
 require(component->getState(&saved)==kResultOk,"VST3 getState failed");
 const auto* raw=static_cast<const std::uint8_t*>(saved.getData());
 drumator::KitState savedKit;
 require(drumator::KitCodec::decode(raw,saved.getCursor(),savedKit),
         "Complete DMR4 kit not saved");
 require(savedKit.wav[0]==makeWav(32,true,-.5f),
         "Original FLOAT32 WAV was not embedded in song state");
 require(savedKit.names[0]=="StudioDrum.wav","WAV filename missing from song");
 require(savedKit.visible==4,"Default four visible rows lost");
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
 require(std::abs(restoredMean/140.+.5)<0.015,
         "Project reload did not reproduce FLOAT32 WAV audio level");
 require(restoredAudio->setProcessing(false)==kResultOk,"Second stop processing");
 require(restored->setActive(false)==kResultOk,"Second deactivate");
 require(restoredController->terminate()==kResultOk,"Second controller terminate");
 require(restored->terminate()==kResultOk,"Second component terminate");
 restoredAudio->release();restoredController->release();restored->release();
 std::cout<<"DMR4 actual fresh-instance audio recall -0.5 PASS\n";

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
 require(send(retainedImport,0)==kResultOk,
         "FLAC-derived WAV rejected by packaged VST3");
 for(int i=0;i<7;++i)run(-1);
 run(36);
 double fromFlac=0;
 for(int i=30;i<100;++i)fromFlac+=left[i];
 require(std::abs(fromFlac/70.-.25)<.015,
         "FLAC-derived sample played at wrong amplitude");
 std::cout<<"M26 real FLAC decoder and packaged VST3 playback PASS\\n";
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
