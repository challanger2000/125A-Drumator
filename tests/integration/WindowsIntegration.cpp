// Windows-only real Controller/View/Processor integration. Probe calls are
// inserted in generated copies; shipped sources and VST3 ABI remain untouched.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <malloc.h>
#include "Probe.h"
#include "ByteStream.h"
#include "vst3/DrumatorController.h"
#include "vst3/DrumatorView.h"
#include "vst3/DrumatorProcessor.h"
#include "vst3/DrumatorEditor.h"
#include "vst3/PluginIDs.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/utility/memoryibstream.h"
#include "vstgui/lib/vstguiinit.h"
#include "vstgui/lib/platform/platformfactory.h"
#include "vstgui/lib/platform/win32/win32factory.h"
#include "vstgui/lib/controls/ctextedit.h"
#include "vstgui/lib/platform/iplatformframe.h"
#include <array>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>
#include <cmath>

void* moduleHandle=nullptr; // SDK moduleinit platform handle, EXE fixture only.
namespace {
thread_local long allocationFail=-1;
thread_local long allocationCount=0;
void allocationPoint() {
 ++allocationCount;
 if(allocationFail==0){allocationFail=-1;throw std::bad_alloc{};}
 if(allocationFail>0)--allocationFail;
}
void check(bool v,const char* msg){if(!v)throw std::runtime_error(msg);}
std::atomic<int> stopPoint{drumator::qa::none};
std::atomic<bool> probeTimeout{false};
HANDLE entered=nullptr,resume=nullptr;
void arm(drumator::qa::Point p) {
 ResetEvent(entered);ResetEvent(resume);probeTimeout=false;stopPoint=p;
}
void waitEntered(){check(WaitForSingleObject(entered,10000)==WAIT_OBJECT_0,"probe not reached");}
void releaseProbe(){stopPoint=drumator::qa::none;SetEvent(resume);}
void pump() {
 MSG m{};while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}
}
void evidence(const std::string& line) {
 std::cout<<line<<'\n';
 char file[32768]{};
 const auto n=GetEnvironmentVariableA("GITHUB_STEP_SUMMARY",file,static_cast<DWORD>(sizeof(file)));
 if(n>0&&n<sizeof(file)){std::ofstream f(file,std::ios::app);f<<"- "<<line<<'\n';}
}
}
void* operator new(std::size_t n){allocationPoint();if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc{};}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace drumator::qa {
void probe(Point p) noexcept {
 if(stopPoint.load(std::memory_order_acquire)!=p)return;
 SetEvent(entered);
 if(WaitForSingleObject(resume,10000)!=WAIT_OBJECT_0)probeTimeout=true;
}
}
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace drumator {
// Added as a friend in generated TEST headers only. Observes real state,
// including Undo/clipboard/paths, rather than mirroring the commit algorithm.
struct IntegrationAccess {
 static auto snapshot(Controller& c) {
   std::array<PatternSteps,kPatterns> undo{};
   std::array<bool,kPatterns> available{};
   for(std::size_t p=0;p<kPatterns;++p){available[p]=c.undo_.available(p);if(auto* x=c.undo_.snapshot(p))undo[p]=*x;}
   std::array<std::pair<std::uint8_t,std::uint8_t>,SongChain::kCapacity> chain{};
   for(std::size_t n=0;n<SongChain::kCapacity;++n){const auto e=c.songChain_.entry(n);chain[n]={e.pattern,e.steps};}
   return std::tuple{c.grid_,c.patternLengths_,c.labels_,c.customNames_,c.samplePaths_,
     c.muted_,c.solo_,c.gain_,c.pan_,c.pitch_,c.choke_,c.output_,c.visibleRows_,
     c.swingPercent_,c.humanizePercent_,c.masterVolume_,c.randomVelocity_,c.songEnabled_,
     c.clipboard_,c.clipboardValid_,c.strokeBefore_,c.strokePattern_,c.strokeGeneration_,
     c.editGeneration(),available,undo,chain,c.songChain_.count(),c.velocityVariation_,c.preferredZoom_,c.getParamNormalized(kPatternParam)};
 }
 static auto viewSnapshot(DrumatorView& v) {
   VSTGUI::IPlatformTextEdit* platform=nullptr;
   if(v.nameEdit_)platform=v.nameEdit_->getPlatformTextEdit();
   return std::tuple{v.laneDragActive_,v.laneDragGeneration_,v.editingNameRow_,v.nameEditGeneration_,
     v.fillMenuRow_,v.getViewSize(),static_cast<const void*>(platform),v.laneDragPattern_,v.laneErase_,
     v.velocityRow_,v.velocityStep_,v.firstRow_,v.songSlot_,v.songEditorOpen_,v.velocityLaneOpen_,
     v.compactViewport_,v.compactPageRows_,v.status_};
 }
 static DrumatorView* view(Controller& c,std::size_t n){check(n<c.views_.size(),"real custom view absent");return c.views_[n];}
 static VSTGUI::CTextEdit* edit(DrumatorView& v){return v.nameEdit_;}
 static const auto& paths(Controller& c){return c.samplePaths_;}
};
}
namespace {
using namespace drumator;
struct ResizeHost final:IPlugFrame {
 bool accept=true;unsigned calls=0;HWND hwnd=nullptr;
 tresult PLUGIN_API queryInterface(const TUID requestedIid,void** obj) override {
   if(FUnknownPrivate::iidEqual(requestedIid,IPlugFrame::iid)||FUnknownPrivate::iidEqual(requestedIid,FUnknown::iid)){
     *obj=static_cast<IPlugFrame*>(this);addRef();return kResultOk;
   }*obj=nullptr;return kNoInterface;
 }
 uint32 PLUGIN_API addRef() override{return 1;}
 uint32 PLUGIN_API release() override{return 1;}
 tresult PLUGIN_API resizeView(IPlugView* view,ViewRect* rect) override {
   ++calls;if(!accept)return kResultFalse;
   if(hwnd)SetWindowPos(hwnd,nullptr,0,0,rect->getWidth(),rect->getHeight(),SWP_NOMOVE|SWP_NOZORDER);
   return view->onSize(rect);
 }
};
struct Fixture {
 HostApplication host;
 Controller* c=new Controller;
 Processor* p=new Processor;
 std::vector<DrumatorEditor*> editors;
 std::array<ResizeHost,3> frames{};
 Fixture(){
   check(c->initialize(&host)==kResultOk&&p->initialize(&host)==kResultOk,"initialize real components");
   check(c->connect(p)==kResultOk&&p->connect(c)==kResultOk,"connect real components");
 }
 ~Fixture(){
   allocationFail=-1;releaseProbe();
   while(!editors.empty())closeLast();
   c->disconnect(p);p->disconnect(c);c->terminate();p->terminate();c->release();p->release();
 }
 void closeLast(){
   const auto n=editors.size()-1;auto* e=editors.back();editors.pop_back();
   static_cast<IPlugView*>(e)->removed();static_cast<IPlugView*>(e)->setFrame(nullptr);e->release();DestroyWindow(frames[n].hwnd);frames[n].hwnd=nullptr;
 }
 DrumatorEditor& open(){
   const auto n=editors.size();check(n<frames.size(),"too many fixture views");
   auto* e=dynamic_cast<DrumatorEditor*>(c->createView(ViewType::kEditor));check(e!=nullptr,"create real editor");
   auto& h=frames[n];h.hwnd=CreateWindowExW(0,L"STATIC",L"Drumator native QA",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
     0,0,1280,850,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
   check(h.hwnd!=nullptr,"create parent HWND");static_cast<IPlugView*>(e)->setFrame(&h);
   check(static_cast<IPlugView*>(e)->attached(h.hwnd,kPlatformTypeHWND)==kResultOk,"attach actual VSTGUI editor");
   editors.push_back(e);pump();return *e;
 }
 static std::vector<std::uint8_t> encode(const KitState& kit){std::vector<std::uint8_t> b;check(KitCodec::encode(kit,b),"valid kit encode");return b;}
 tresult recall(const std::vector<std::uint8_t>& bytes){
   drumator::qa::ByteStream stream(const_cast<std::uint8_t*>(bytes.data()),bytes.size());return c->setComponentState(&stream);
 }
 void kit(const KitState& kit){
   auto b=encode(kit);drumator::qa::ByteStream stream(b.data(),b.size());
   check(p->setState(&stream)==kResultOk&&recall(b)==kResultOk,"load real processor/controller");
 }
};
KitState namedKit(char ch,std::uint8_t rows=4) {
 KitState k;k.visible=rows;k.grid.fill(0);k.grid[0]=ch=='X'?37:93;
 if(ch!='X'){k.gain[0]=130;k.pan[0]=125;k.pitch[0]=22;k.masterVolume=75;}
 for(std::size_t r=0;r<kRows;++r){k.displayNames[r]=std::string(40,ch);k.names[r]=std::string(48,ch)+".wav";}
 return k;
}
tresult zoom(Controller& c,bool large){
 std::array<std::uint8_t,5> bytes{{0x44,0x5a,0x4d,0x31,static_cast<std::uint8_t>(large)}};
 drumator::qa::ByteStream s(bytes.data(),bytes.size());return c.setState(&s);
}
void click(DrumatorView& v,double x,double y,int buttons=VSTGUI::kLButton){
 VSTGUI::CPoint at(x,y);v.onMouseDown(at,VSTGUI::CButtonState(buttons));
}
HWND nameEditor(Fixture& f,std::size_t view=0,std::size_t row=0){
 SetActiveWindow(f.frames[view].hwnd);SetForegroundWindow(f.frames[view].hwnd);
 SetFocus(static_cast<HWND>(f.editors[view]->getFrame()->getPlatformFrame()->getPlatformRepresentation()));pump();
 auto* v=IntegrationAccess::view(*f.c,view);
 click(*v,100,108+static_cast<double>(row)*29+8,VSTGUI::kLButton|VSTGUI::kShift);
 pump();auto h=GetFocus();wchar_t cls[32]{};GetClassNameW(h,cls,32);
 check(h&&lstrcmpiW(cls,L"EDIT")==0,"native EDIT HWND/focus missing");
 check(IntegrationAccess::edit(*v)->getPlatformTextEdit()!=nullptr,"CTextEdit platform absent");return h;
}
void testPatternOverlap(){
 AtomicPatternBank bank;std::array<std::uint8_t,kRows*kSteps> x{},y{},actual{};x.fill(15);y.fill(115);
 check(bank.replacePattern(0,x.data(),x.size()),"T03 initial pattern");
 for(int i=0;i<16;++i){
   arm(qa::patternMidWrite);
   std::jthread writer([&]{bank.replacePattern(0,y.data(),y.size());});waitEntered();
   const bool accepted=bank.coherentPattern(0,actual);
   std::array<std::uint8_t,kRows*kSteps> torn{};
   for(std::size_t n=0;n<torn.size();++n)torn[n]=bank.get(0,n/kSteps,n%kSteps);
   const bool oracleRejects=torn!=x&&torn!=y;
   releaseProbe();writer.join();
   check(!accepted,"T03 read accepted an in-progress writer");
   check(oracleRejects,"T03 torn-reader negative was not truly mixed");
   check(bank.coherentPattern(0,actual)&&actual==y,"T03 completed generation absent");
   arm(qa::patternReadStarted);bool got=true;
   std::jthread reader([&]{got=bank.coherentPattern(0,actual);});waitEntered();
   bank.replacePattern(0,x.data(),x.size());releaseProbe();reader.join();
   check(!got,"T03 reader accepted changed generation");
   check(bank.coherentPattern(0,actual)&&actual==x,"T03 quiescent X missing");
 }
 evidence("T03 PASS: 16 mid-write rejections, 16 interrupted-read rejections, 32 complete reads, 16 real torn-reader negatives");
}
void prepareAudio(Processor& p){
 SpeakerArrangement stereo=SpeakerArr::kStereo;check(p.setBusArrangements(nullptr,0,&stereo,1)==kResultOk,"audio buses");
 ProcessSetup s{};s.sampleRate=48000;s.maxSamplesPerBlock=256;s.symbolicSampleSize=kSample32;s.processMode=kRealtime;
 check(p.setupProcessing(s)==kResultOk&&p.setActive(true)==kResultOk&&p.setProcessing(true)==kResultOk,"prepare actual audio");
}
std::vector<std::uint8_t> constantStereo(float l,float r){
 std::vector<std::uint8_t> b(44+1024*8,0);
 auto put=[&](std::size_t at,std::uint32_t x,unsigned n){for(unsigned i=0;i<n;++i)b[at+i]=static_cast<std::uint8_t>(x>>(i*8));};
 std::memcpy(b.data(),"RIFF",4);put(4,static_cast<std::uint32_t>(b.size()-8),4);
 std::memcpy(b.data()+8,"WAVEfmt ",8);put(16,16,4);put(20,3,2);put(22,2,2);
 put(24,48000,4);put(28,48000*8,4);put(32,8,2);put(34,32,2);
 std::memcpy(b.data()+36,"data",4);put(40,1024*8,4);
 for(std::size_t n=0;n<1024;++n){std::memcpy(b.data()+44+n*8,&l,4);std::memcpy(b.data()+48+n*8,&r,4);}
 return b;
}
void testArbitration(bool zero,bool busy,const std::vector<unsigned>& points){
 Fixture f;auto old=namedKit('X');old.selected=2;f.kit(old);prepareAudio(*f.p);
 auto next=namedKit('Y');next.selected=6;next.grid.fill(0);
 next.gain[0]=100;next.pan[0]=100;next.pitch[0]=24;next.masterVolume=100;
 next.wav[0]=constantStereo(.4f,-.2f);
 for(std::size_t p=0;p<kPatterns;++p)next.grid[p*kRows*kSteps]=static_cast<std::uint8_t>(16+p*16);
 auto bytes=Fixture::encode(next);
 arm(busy?qa::restoreMidCommit:qa::restoreSnapshot);
 tresult restored=kResultFalse;
 std::jthread restore([&]{drumator::qa::ByteStream s(bytes.data(),bytes.size());restored=f.p->setState(&s);});waitEntered();
 std::array<float,256> left{},right{};float* ptrs[]{left.data(),right.data()};
 AudioBusBuffers out{};out.numChannels=2;out.channelBuffers32=ptrs;
 ProcessData data{};data.numSamples=zero?0:256;data.symbolicSampleSize=kSample32;
 data.numOutputs=zero?0:1;data.outputs=zero?nullptr:&out;
 tresult processed=kResultOk;
 // Distinct process calls are essential: SDK queues coalesce equal offsets.
 // In a zero-frame flush every valid point is at offset zero.
 for(auto point:points){
   ParameterChanges changes;int32 index=0;
   changes.addParameterData(kPatternParam,index)->addPoint(0,static_cast<double>(point)/7.,index);
   data.inputParameterChanges=&changes;
   if(f.p->process(data)!=kResultOk)processed=kResultFalse;
 }
 data.inputParameterChanges=nullptr;releaseProbe();restore.join();
 check(processed==kResultOk&&restored==kResultOk&&!probeTimeout,"O01 execution/controlled barrier failed");
 ResizableMemoryIBStream saved;KitState observed;
 check(f.p->getState(&saved)==kResultOk&&KitCodec::decode(static_cast<const std::uint8_t*>(saved.getData()),saved.getCursor(),observed),"O01 returned state");
 // Documented AtomicPatternSelection contract: host points after restore's
 // selection snapshot must not be overwritten, including equal values/ABA.
 check(observed.selected==points.back(),"O01 later host automation overwritten by restore");
 saved.rewind();
 check(f.c->setComponentState(&saved)==kResultOk&&f.c->currentPattern()==points.back(),"O01 controller/host recall mismatch");
 String128 label{};check(f.c->getParamStringByValue(kPatternParam,f.c->getParamNormalized(kPatternParam),label)==kResultOk&&
   label[0]==static_cast<TChar>('A'+points.back()),"O01 host label differs");
 // Read the audible selected pattern independently of the returned state.
 ProcessContext clock{};clock.tempo=120;clock.projectTimeMusic=0;
 clock.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 data.numSamples=256;data.numOutputs=1;data.outputs=&out;data.processContext=&clock;
 check(f.p->process(data)==kResultOk,"O01 follow-up render");
 const double velocity=static_cast<double>(16+points.back()*16)/127.;
 for(std::size_t n=80;n<180;++n)
   check(std::abs(left[n]-.4*velocity)<2e-4&&std::abs(right[n]+.2*velocity)<2e-4,
         "O01 audible selected pattern differs from state/host");
 f.p->setProcessing(false);f.p->setActive(false);
}
void testKitGate(){
 Fixture f;auto old=namedKit('X');old.wav[0]=constantStereo(.25f,-.35f);
 old.grid.fill(0);old.grid[0]=127;f.kit(old);prepareAudio(*f.p);
 auto next=namedKit('Y');next.selected=1;next.grid.fill(0);next.grid[kRows*kSteps]=64;
 next.gain[0]=100;next.pan[0]=100;next.pitch[0]=24;next.masterVolume=100;
 next.wav[0]=constantStereo(-.4f,.1f);auto bytes=Fixture::encode(next);
 arm(qa::restoreMidCommit);tresult loaded=kResultFalse;
 std::jthread control([&]{qa::ByteStream s(bytes.data(),bytes.size());loaded=f.p->setState(&s);});waitEntered();
 std::array<float,256> l{},r{};l.fill(19);r.fill(19);float* ptr[]{l.data(),r.data()};
 AudioBusBuffers bus{};bus.numChannels=2;bus.channelBuffers32=ptr;
 ProcessContext clock{};clock.tempo=120;clock.state=ProcessContext::kPlaying|
   ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 ProcessData audio{};audio.symbolicSampleSize=kSample32;audio.numSamples=256;
 audio.numOutputs=1;audio.outputs=&bus;audio.processContext=&clock;
 const auto result=f.p->process(audio);releaseProbe();control.join();
 check(result==kResultOk&&loaded==kResultOk&&!probeTimeout,"T04 forced mid-commit execution");
 for(std::size_t n=0;n<256;++n)check(l[n]==0&&r[n]==0,"T04 busy gate leaked partially installed kit");
 check(f.p->process(audio)==kResultOk,"T04 after gate render");
 for(std::size_t n=80;n<180;++n)
   check(std::abs(l[n]+.4*64./127.)<2e-4&&std::abs(r[n]-.1*64./127.)<2e-4,
     "T04 first audible block mixed sample/pattern/mix generations");
 f.p->setProcessing(false);f.p->setActive(false);
 evidence("T04 controlled gate PASS: real process paused inside restore, exact silence then coherent new-kit audio");
}
void activeEdits(Fixture& f){
 auto* v=IntegrationAccess::view(*f.c,0);
 // Actual Undo and clipboard plus an active drag in a real view.
 check(f.c->setStepVelocity(0,0,3,91)&&f.c->copyCurrentPattern(),"Z01 create Undo/clipboard");
 click(*v,80,108+4*29+5+45); // open velocity lane using the real button
 click(*v,630,108+4*29+100);
 check(std::get<0>(IntegrationAccess::viewSnapshot(*v)),"Z01 fixture did not start actual view drag");
 check(f.c->setStepVelocity(0,0,4,85),"Z01 active stroke change");
}
void testRecallOOM(){
 // Count actual allocations in the real setComponentState, then fault each
 // reached position. Use fresh controllers so previous failures cannot hide one.
 long sites=0;auto y=namedKit('Y');auto bytes=Fixture::encode(y);
 {Fixture f;f.kit(namedKit('X'));f.open();f.open();activeEdits(f);SetWindowTextW(nameEditor(f),L"OLD PENDING BEFORE RECALL");
  allocationCount=0;check(f.recall(bytes)==kResultOk,"Z01 count recall");sites=allocationCount;}
 check(sites>=32,"Z01 injection did not reach allocating controller path");
 unsigned rejected=0,accepted=0;
 for(long fault=0;fault<sites;++fault){
   Fixture f;f.kit(namedKit('X'));f.open();f.open();activeEdits(f);const auto h=nameEditor(f);SetWindowTextW(h,L"OLD PENDING BEFORE RECALL");
   auto* v=IntegrationAccess::view(*f.c,0);auto before=IntegrationAccess::snapshot(*f.c);
   auto viewBefore=IntegrationAccess::viewSnapshot(*v);
   allocationFail=fault;tresult result=kResultFalse;bool escaped=false;
   try {result=f.recall(bytes);}catch(const std::bad_alloc&){escaped=true;}
   allocationFail=-1;
   check(!escaped,"Z01 bad_alloc escaped the actual controller recall");
   if(result!=kResultOk){
     ++rejected;check(IntegrationAccess::snapshot(*f.c)==before,"Z01 rejected recall changed data/Undo/gesture");
     check(IntegrationAccess::viewSnapshot(*v)==viewBefore&&IsWindow(h)&&GetFocus()==h,"Z01 rejected recall changed view/native edit");
     // Cancel must still restore the old, valid kit gesture; Undo remains usable.
     v->onMouseCancel();check(f.c->canUndoCurrentPattern(),"Z01 rejected recall lost Undo");
   }else{
     ++accepted;check(f.c->customName(0)==y.displayNames[0]&&!f.c->canUndoCurrentPattern(),"Z01 accepted recall not committed");
     const auto after=IntegrationAccess::snapshot(*f.c);
     VSTGUI::CPoint pt(630,400);v->onMouseMoved(pt,VSTGUI::CButtonState(VSTGUI::kLButton));v->onMouseUp(pt,{});v->onMouseCancel();
     if(auto* edit=IntegrationAccess::edit(*v))v->valueChanged(edit);
     check(IntegrationAccess::snapshot(*f.c)==after,"Z01 late callback changed accepted kit");
   }
 }
 check(rejected>0,"Z01 no real rejected allocation");
 evidence("Z01 PASS: actual Controller OOM sites="+std::to_string(sites)+", rejected="+std::to_string(rejected)+", postcommit accepted="+std::to_string(accepted));
}
void testLegacyReject(){
 for(std::uint32_t magic:{0x31524d44u,0x32524d44u,0x33524d44u})for(long fault=0;fault<16;++fault){
   Fixture f;f.kit(namedKit('X'));f.open();f.open();activeEdits(f);
   std::vector<std::uint8_t> b;
   for(auto n:{magic,std::uint32_t{0}})for(int i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(n>>(i*8)));
   if(magic==0x33524d44u)b.push_back(4);
   if(magic!=0x31524d44u)b.insert(b.end(),kCells,19);
   const auto before=IntegrationAccess::snapshot(*f.c);
   allocationFail=fault;const auto result=f.recall(b);allocationFail=-1;
   check(result!=kResultOk&&IntegrationAccess::snapshot(*f.c)==before,"Z01 rejected Legacy changed state");
   check(f.recall(b)==kResultOk&&!f.c->canUndoCurrentPattern(),"Z01 successful Legacy failed");
 }
 evidence("Z01 Legacy PASS: DMR1/2/3 48 actual default-name OOM rejections and successful recalls");
}
void testZoom(){
 Fixture f;f.kit(namedKit('X'));auto& e=f.open();auto* v=IntegrationAccess::view(*f.c,0);
 for(double dpi:{1.,1.5,2.}){
   check(e.setContentScaleFactor(dpi)==kResultOk,"Z02 attached DPI rejected");
   for(bool large:{true,false}){
     const auto before=e.getFrame()->getViewSize();const auto transform=e.getFrame()->getTransform();
     const auto preference=f.c->preferredZoom();f.frames[0].accept=false;
     check(zoom(*f.c,large)!=kResultOk,"Z02 host rejection accepted");
     check(e.getFrame()->getViewSize()==before&&e.getFrame()->getTransform()==transform&&
       f.c->preferredZoom()==preference,"Z02 host rejection partially changed zoom");
     f.frames[0].accept=true;check(zoom(*f.c,large)==kResultOk,"Z02 accepted recall failed");
     check(v->zoomMatches(large?1.5:1.)&&std::abs(e.getFrame()->getTransform().m11-dpi*(large?1.5:1.))<.001,"Z02 transform mismatch");
   }
 }
 check(zoom(*f.c,true)==kResultOk,"Z02 saved preference set");
 ResizableMemoryIBStream saved;
 check(f.c->getState(&saved)==kResultOk&&saved.getCursor()==5&&
   static_cast<const std::uint8_t*>(saved.getData())[4]==1,"Z02 saved zoom bytes disagree");
 f.closeLast();
 auto& reopened=f.open();check(std::abs(reopened.getZoomFactor()-1.5)<.001,"Z02 reopen lost preference");
 // Both views need a change: policy must reject before any native host request.
 f.open();const auto calls=f.frames[0].calls+f.frames[1].calls;
 check(zoom(*f.c,false)!=kResultOk&&f.c->preferredZoom()==1.5,"Z02 multi-view recall not rejected");
 check(f.frames[0].calls+f.frames[1].calls==calls&&
   IntegrationAccess::view(*f.c,0)->zoomMatches(1.5)&&IntegrationAccess::view(*f.c,1)->zoomMatches(1.5),"Z02 partial multi-view change");
 evidence("Z02 PASS: attached 100/150, DPI 100/150/200, host refusals, saved reopen and atomic two-view rejection");
}
void testNativeName(){
 for(auto key:{VK_RETURN,VK_ESCAPE,0}){
   Fixture f;f.kit(namedKit('X',16));f.open();f.open();
   auto old=nameEditor(f,0,10);SetWindowTextW(old,L"OLD PENDING NAME");
   const auto token=f.c->editGeneration();
   auto bad=Fixture::encode(namedKit('Y'));bad.back()^=1;
   check(f.recall(bad)!=kResultOk&&IsWindow(old)&&f.c->editGeneration()==token,"Z03 rejected recall closed editor");
   check(f.recall(Fixture::encode(namedKit('Y',4)))==kResultOk&&!IsWindow(old),"Z03 successful recall retained old HWND");
   auto* v=IntegrationAccess::view(*f.c,0);const auto name=f.c->customName(0);
   const auto committed=IntegrationAccess::snapshot(*f.c);
   auto* edit=IntegrationAccess::edit(*v);check(edit!=nullptr,"Z03 CTextEdit unexpectedly deleted");
   v->valueChanged(edit);v->onTextEditPlatformControlLostFocus(edit);
   check(IntegrationAccess::snapshot(*f.c)==committed,"Z03 delayed callback renamed new kit");
   auto fresh=nameEditor(f,1,0);SetWindowTextW(fresh,L"FRESH NAME");
   if(key)SendMessageW(fresh,WM_KEYDOWN,static_cast<WPARAM>(key),0);
   else SetFocus(f.frames[1].hwnd);
   pump();
   check(!IsWindow(fresh),"Z03 native editor survived key/focus finish");
   check(f.c->customName(0)==(key==VK_ESCAPE?name:std::string("FRESH NAME")),"Z03 Enter/Escape/blur name contract");
   auto closing=nameEditor(f,1,0);f.closeLast();check(!IsWindow(closing),"Z03 remove kept HWND");
 }
 evidence("Z03 PASS: real Win32 EDIT lifetime, rejected/accepted 16-to-4 recall, late live-listener callbacks, Enter/Escape/blur, two views and close");
}
}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 moduleHandle=GetModuleHandleW(nullptr);VSTGUI::init(static_cast<HINSTANCE>(moduleHandle));
 auto* platform=VSTGUI::getPlatformFactory().asWin32Factory();
 if(!platform)return 3;platform->setResourceBasePath(VSTGUI::UTF8String(argv[1]));
 entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 int failures=0;
 auto run=[&](const std::string& name,const auto& test){
   try{test();evidence(name+" PASS");}
   catch(const std::exception& ex){allocationFail=-1;releaseProbe();++failures;evidence(name+" FAIL: "+ex.what());}
 };
 run("T03 controlled actual bank",testPatternOverlap);
 run("T04 controlled real restore gate",testKitGate);
 for(bool zero:{true,false})for(bool busy:{false,true})for(const auto& points:std::vector<std::vector<unsigned>>{{5},{2},{5,2}}){
   const std::string name="O01 "+std::string(zero?"zero":"audio")+(busy?" busy ":" snapshot ")+
     (points.size()>1?"ABA":points[0]==2?"equal":"different");
   run(name,[&]{testArbitration(zero,busy,points);});
 }
 run("Z01 full Controller allocation injection",testRecallOOM);
 run("Z01 legacy Controller",testLegacyReject);
 run("Z02 native attached views",testZoom);
 run("Z03 Win32 name editor",testNativeName);
 evidence("Integration total failures: "+std::to_string(failures)+"; Studio One NOT RUN");
 CloseHandle(entered);CloseHandle(resume);VSTGUI::exit();CoUninitialize();return failures?1:0;
}
