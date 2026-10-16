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
