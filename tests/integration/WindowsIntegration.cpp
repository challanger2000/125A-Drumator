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
#include "ResultLog.h"
#include "core/PatternQuantization.h"
#include "core/Utf8Path.h"
#include "core/FlacImport.h"
#include "vst3/DrumatorController.h"
#include "vst3/DrumatorView.h"
#include "core/UiDetailGeometry.h"
#include "core/UiRowControls.h"
#include "core/UiCompactLayout.h"
#include "core/UiSongLayout.h"
#include "vst3/DrumatorProcessor.h"
#include "vst3/DrumatorEditor.h"
#include "vst3/PluginIDs.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/utility/memoryibstream.h"
#include "vstgui/lib/vstguiinit.h"
#include "vstgui/lib/platform/platformfactory.h"
#include "vstgui/lib/platform/win32/win32factory.h"
#include "vstgui/lib/controls/ctextedit.h"
#include "vstgui/lib/platform/iplatformframe.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/coffscreencontext.h"
#include "vstgui/lib/idatapackage.h"
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <array>
#include <limits>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
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
thread_local bool legacyTriggerOrdering=false;
std::atomic<AuditMutation> auditMutation{AuditMutation::none};
std::atomic<bool> suppressRotaryFace{false};
std::atomic<unsigned> auditSongScheduleCalls{0};
std::atomic<unsigned> auditSongPatternSnapshots{0};
std::atomic<unsigned> auditViewRefreshFaultHits{0};
std::atomic<unsigned> auditWaveDecodeMisses{0},auditWaveFullReplies{0};
std::atomic<unsigned> auditWaveIndexedCalls{0};
std::atomic<const void*> auditImportTarget{nullptr};
std::atomic<unsigned> auditImportCalls{0},auditDelayCalls{0};
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
 static bool sampleEditorOpen(const DrumatorView& view){return view.sampleEditorOpen_;}
 static std::string sampleEditorStatus(const DrumatorView& v){return v.status_;}
 static bool openSampleEditorForCapture(DrumatorView& view,std::size_t row){return view.openSampleEditor(row);}
 static bool importPathViaView(DrumatorView& view,std::size_t row,const std::string& path){return view.loadPath(row,path);}
 static std::pair<std::uint32_t,std::uint32_t> editorViewport(const DrumatorView& view){return {view.sampleEditorFirst_,view.sampleEditorLast_};}
 static const auto& editorLeft(const DrumatorView& view){return view.sampleEditorWave_;}
 static const auto& editorRight(const DrumatorView& view){return view.sampleEditorWaveRight_;}
 static bool editorStereo(const DrumatorView& view){return view.sampleEditorStereo_;}
 static void pollMeter(DrumatorView& view){view.pollMeter();}
 static std::array<float,4> meterPaintState(const DrumatorView& v){return {v.mainMeterL_,v.mainMeterR_,v.holdMeterL_,v.holdMeterR_};}
 static std::uint32_t editorFrameCount(const DrumatorView& view){return view.sampleEditorFrames_;}
 static std::array<double,3> rotaryImageStatus(const DrumatorView& view){
   const auto* bmp=view.knobBitmap_;
   return {bmp&&bmp->isLoaded()?1.:0.,bmp?bmp->getWidth():-1.,bmp?bmp->getHeight():-1.};
 }
 static SampleEdit sampleEditorDraft(const DrumatorView& view){return view.sampleEditorEdit_;}
 static void setSampleEditorDraft(DrumatorView& view,const SampleEdit& edit){view.sampleEditorEdit_=edit;}
 static void setStatusText(DrumatorView& view,const std::string& status){view.status_=status;}
 static void seedTransportClock(Processor& p) {
   p.haveExpectedPpq_=true;
   p.expectedNextPpq_=19.5;
   p.phaseOriginStep_=76;
 }
 static bool clockReset(const Processor& p) {
   return !p.haveExpectedPpq_&&p.expectedNextPpq_==0.0&&p.phaseOriginStep_==0;
 }
 static std::uint32_t activeAudioVoices(const Processor& p) {
   return p.voices_.active();
 }
 static auto snapshot(Controller& c) {
   std::array<PatternSteps,kPatterns> undo{};
   std::array<bool,kPatterns> available{};
   std::array<std::uint8_t,kPatterns> undoLengths{};
   for(std::size_t p=0;p<kPatterns;++p){
     available[p]=c.undo_.available(p);
     undoLengths[p]=c.undo_.snapshotLength(p);
     if(auto* x=c.undo_.snapshot(p))undo[p]=*x;
   }
   std::array<std::pair<std::uint8_t,std::uint8_t>,SongChain::kCapacity> chain{};
   for(std::size_t n=0;n<SongChain::kCapacity;++n){const auto e=c.songChain_.entry(n);chain[n]={e.pattern,e.steps};}
   return std::tuple{c.grid_,c.patternLengths_,c.labels_,c.customNames_,c.samplePaths_,
     c.muted_,c.solo_,c.gain_,c.pan_,c.pitch_,c.choke_,c.output_,c.visibleRows_,
     c.swingPercent_,c.humanizePercent_,c.masterVolume_,c.randomVelocity_,c.songEnabled_,
     c.clipboard_,c.clipboardLength_,c.clipboardValid_,c.strokeBefore_,c.strokePattern_,c.strokeGeneration_,
     c.editGeneration(),available,undo,undoLengths,chain,c.songChain_.count(),c.velocityVariation_,c.preferredZoom_,c.getParamNormalized(kPatternParam),
     c.originIdentity_,c.originIdentityValid_,c.pathsRestoredBeforeComponent_,c.strokeOwner_};
 }
 static auto viewSnapshot(DrumatorView& v) {
   VSTGUI::IPlatformTextEdit* platform=nullptr;
   if(v.nameEdit_)platform=v.nameEdit_->getPlatformTextEdit();
   return std::tuple{v.laneDragActive_,v.laneDragGeneration_,v.editingNameRow_,v.nameEditGeneration_,
     v.fillMenuRow_,v.getViewSize(),static_cast<const void*>(platform),v.laneDragPattern_,v.laneErase_,
      v.detailDragControl_,v.detailDragRow_,
     v.velocityRow_,v.velocityStep_,v.firstRow_,v.songSlot_,v.songEditorOpen_,
     v.compactViewport_,v.compactPageRows_,v.velocityExpanded_,v.popup_,v.popupRow_,v.activeKnob_,v.knobRow_,v.knobStartY_,v.knobStartValue_,v.knobGeneration_,v.status_};
 }
 static std::size_t viewCount(const Controller& c){return c.views_.size();}
 static DrumatorView* view(Controller& c,std::size_t n){check(n<c.views_.size(),"real custom view absent");return c.views_[n];}
 static VSTGUI::CTextEdit* edit(DrumatorView& v){return v.nameEdit_;}
  static std::size_t firstRow(DrumatorView& v){return v.firstRow_;}
  static std::size_t selectedRow(DrumatorView& v){return v.velocityRow_;}
  static double rowTop(DrumatorView& v,std::size_t i){return v.rowTopOnPage(i);}
  static bool velocityExpanded(DrumatorView& v,std::size_t row){return v.velocityExpanded_[row];}
  static int activeKnob(DrumatorView& v){return v.activeKnob_;}
  static int popup(DrumatorView& v){return v.popup_;}
  static std::size_t popupRow(DrumatorView& v){return v.popupRow_;}
 static std::size_t songPage(DrumatorView& v){return v.songPage_;}
 static std::size_t songSlot(DrumatorView& v){return v.songSlot_;}
 static const auto& paths(Controller& c){return c.samplePaths_;}
 static bool pendingDirty(const Controller& c){return c.dirtyPending_;}
 static bool zoomSample(DrumatorView& v,bool in,bool full=false){return v.zoomSampleEditor(in,full);}
 static bool scrollSample(DrumatorView& v,double x){return v.scrollEditorWave(x);}
 static bool waveCacheEmpty(const Controller& c){return c.editorWaveRow_==kRows;}
 static std::size_t cachedWaveRow(const Controller& c){return c.editorWaveRow_;}
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
 while(IntegrationAccess::firstRow(*v)+UiCompactLayout::perPage<=row){
    const auto bottom=v->getViewSize().getHeight()-UiCompactLayout::footer;
    click(*v,1190,bottom+17.);
  }
  check(row>=IntegrationAccess::firstRow(*v)&&
        row<IntegrationAccess::firstRow(*v)+UiCompactLayout::perPage,
        "name editor row not in viewport");
  click(*v,110,IntegrationAccess::rowTop(*v,row-IntegrationAccess::firstRow(*v))+14.,
        VSTGUI::kLButton|VSTGUI::kShift);
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
 std::array<SpeakerArrangement,kRows> allOutputs{};
 allOutputs.fill(SpeakerArr::kStereo);
 check(p.setBusArrangements(nullptr,0,allOutputs.data(),
                             static_cast<Steinberg::int32>(allOutputs.size()))==kResultOk,
       "16 fixed stereo output arrangements");
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
void testWavEditorPreviewAndRecall(){
 Fixture f;
 KitState initial;initial.visible=2;initial.grid.fill(0);
 initial.names[0]="sample.wav";initial.wav[0]=constantStereo(.5f,-.25f);
 f.kit(initial);prepareAudio(*f.p);
 std::array<float,256> left{},right{};float* channels[]{left.data(),right.data()};
 AudioBusBuffers out{};out.numChannels=2;out.channelBuffers32=channels;
 ProcessData block{};block.numSamples=256;block.numOutputs=1;block.outputs=&out;
 block.symbolicSampleSize=kSample32;
 auto getKit=[&]{
   ResizableMemoryIBStream stream;KitState kit;
   check(f.p->getState(&stream)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                          static_cast<std::size_t>(stream.getCursor()),kit),
         "WE01 processor save/decode failed");
   return kit;
 };
 SampleEdit candidate{64,512,0,0,.5f};
 SampleEdit observed{};std::uint32_t frames=0;
 std::array<WaveformRange,256> peaks{};
 check(f.c->sampleEditorSnapshot(0,observed,frames,peaks)&&frames==1024&&
       observed.start==0&&observed.end==1024,"WE01 embedded editor snapshot");
 check(f.c->previewSampleEdit(0,candidate),"WE01 provisional preview rejected");
 check(f.p->process(block)==kResultOk,"WE01 stopped-transport preview render");
 check(std::abs(left[100]-.25f)<.005f&&
       std::abs(right[100]+.125f)<.005f,
       "WE01 provisional START/GAIN not audible");
 check(getKit().sampleEdits[0].end==0,"WE01 preview dirtied project");
 f.c->stopSampleEditPreview();
 check(f.p->process(block)==kResultOk,"WE01 preview stop render");
 check(std::abs(left[100])<1e-6f&&std::abs(right[100])<1e-6f,
       "WE01 cancel left provisional audio playing");
 check(getKit().sampleEdits[0].end==0,"WE01 cancel changed saved edit");
 check(f.c->commitSampleEdit(0,candidate),"WE01 commit rejected");
 const auto committed=getKit();
 check(committed.sampleEdits[0].start==64&&committed.sampleEdits[0].end==512&&
       committed.sampleEdits[0].gain==.5f,"WE01 edit missing from kit state");
 const auto encoded=Fixture::encode(committed);
 drumator::qa::ByteStream input(const_cast<std::uint8_t*>(encoded.data()),encoded.size());
 check(f.p->setState(&input)==kResultOk&&f.recall(encoded)==kResultOk,
       "WE01 processor/controller recall rejected");
 check(f.c->sampleEditorSnapshot(0,observed,frames,peaks)&&
       observed.start==64&&observed.end==512&&observed.gain==.5f,
       "WE01 recalled editor state differs");
 // Repeat native editor attach/detach while snapshots and previews are active.
 for(unsigned cycle=0;cycle<3;++cycle){
   f.open();
   check(f.c->sampleEditorSnapshot(0,observed,frames,peaks),
         "WE02 editor re-open lost WAV snapshot");
   check(f.c->previewSampleEdit(0,candidate),"WE02 repeated preview rejected");
   check(f.p->process(block)==kResultOk,"WE02 repeated preview render");
   f.closeLast();
   f.c->stopSampleEditPreview();
   check(f.p->process(block)==kResultOk,"WE02 stopped preview after detach");
   check(std::abs(left[100])<1e-6f,"WE02 orphaned preview after editor close");
 }
 // Valid host transport clock must not suppress a provisional audition.
 ProcessContext clock{};clock.tempo=120.;clock.projectTimeMusic=0.;
 clock.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|
             ProcessContext::kProjectTimeMusicValid;
 block.processContext=&clock;
 check(f.c->previewSampleEdit(0,candidate),"WE03 transport preview rejected");
 check(f.p->process(block)==kResultOk,"WE03 transport preview render");
 check(std::abs(left[100]-.25f)<.005f&&
       std::abs(right[100]+.125f)<.005f,
       "WE03 running transport provisional output wrong");
 check(getKit().sampleEdits[0].start==64,"WE03 preview changed saved kit");
 f.c->stopSampleEditPreview();clock.projectTimeMusic+=256./48000.;
 check(f.p->process(block)==kResultOk,"WE03 transport preview stop");
 check(std::abs(left[100])<1e-6f,"WE03 stopped preview still audible");
 block.processContext=nullptr;
 f.p->setProcessing(false);f.p->setActive(false);
 evidence("WE01-03 provisional audition stopped/running transport, 3 view cycles and DM16 recall PASS");
}
void testArbitration(bool zero,bool busy,const std::vector<unsigned>& points){
 Fixture f;auto old=namedKit('X');old.selected=2;f.kit(old);prepareAudio(*f.p);
 auto next=namedKit('Y');next.selected=6;next.grid.fill(0);
 next.gain[0]=100;next.pan[0]=100;next.pitch[0]=24;next.masterVolume=100;
 next.wav[0]=constantStereo(.4f,-.2f);
 for(std::size_t p=0;p<kPatterns;++p)next.grid[p*kRows*kSteps]=static_cast<std::uint8_t>(12+p*16);
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
 const double velocity=static_cast<double>(12+points.back()*16)/127.;
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
 click(*v,1248,UiCompactLayout::header+88); // expand velocity only for first instrument
 check(IntegrationAccess::velocityExpanded(*v,0),"Z01 Velocity row did not expand");
 click(*v,630,UiCompactLayout::header+UiCompactLayout::baseRow+12); // inline velocity for first instrument
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
void testMultiViewLifecycleNotifications(){
 // V18: attached native views receive controller notifications while
 // both exist; removing one must leave no dangling registration.
 Fixture f;
 f.kit(namedKit('X'));
 f.open();f.open();
 check(IntegrationAccess::viewCount(*f.c)==2,"V18 both views not registered");
 auto* original=IntegrationAccess::view(*f.c,0);
 auto* second=IntegrationAccess::view(*f.c,1);
 check(original!=second,"V18 duplicate view registration");
 check(f.c->setParamNormalized(kPatternParam,3.0/7.0)==kResultOk,
       "V18 dual-view parameter notification");
 check(IntegrationAccess::viewCount(*f.c)==2&&
       IntegrationAccess::view(*f.c,0)==original&&
       IntegrationAccess::view(*f.c,1)==second,
       "V18 view collection corrupted by notification");
 f.closeLast();
 check(IntegrationAccess::viewCount(*f.c)==1&&IntegrationAccess::view(*f.c,0)==original,
       "V18 removed view still registered");
 check(f.c->setParamNormalized(kPatternParam,4.0/7.0)==kResultOk,
       "V18 notification after second view removal");
 check(IntegrationAccess::viewCount(*f.c)==1,"V18 stale view after notification");
 f.open();
 check(IntegrationAccess::viewCount(*f.c)==2&&IntegrationAccess::view(*f.c,0)==original,
       "V18 reopened editor lost original view");
 check(f.c->setParamNormalized(kPatternParam,5.0/7.0)==kResultOk,
       "V18 notification after reopening");
 evidence("V18 two native views, notification, removal, reopening PASS");
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
 // DZM2 preserves intermediate user zoom; DZM1 remains backward compatible.
 for(const auto entry : {std::pair<double,std::uint8_t>{1.2,1},
                         std::pair<double,std::uint8_t>{1.3,2}}) {
   std::array<std::uint8_t,5> v2{{'D','Z','M','2',entry.second}};
   drumator::qa::ByteStream in(v2.data(),v2.size());
   check(f.c->setState(&in)==kResultOk&&v->zoomMatches(entry.first),"Z02 DZM2 intermediate zoom recall");
   ResizableMemoryIBStream written;
   check(f.c->getState(&written)==kResultOk&&written.getCursor()==5+2*kRows,
         "Z02 DZM3 state length with empty sample paths");
   const auto* b=static_cast<const std::uint8_t*>(written.getData());
   check(b[0]=='D'&&b[1]=='Z'&&b[2]=='M'&&b[3]=='3'&&b[4]==entry.second,
         "Z02 intermediate zoom not persisted in DZM3");
 }
 check(zoom(*f.c,true)==kResultOk,"Z02 saved preference set");
 ResizableMemoryIBStream saved;
 check(f.c->getState(&saved)==kResultOk&&saved.getCursor()==5+2*kRows&&
   static_cast<const std::uint8_t*>(saved.getData())[4]==3,
   "Z02 saved DZM3 zoom bytes disagree");
 f.closeLast();
 auto& reopened=f.open();check(std::abs(reopened.getZoomFactor()-1.5)<.001,"Z02 reopen lost preference");
 // Both views need a change: policy must reject before any native host request.
 f.open();const auto calls=f.frames[0].calls+f.frames[1].calls;
 check(zoom(*f.c,false)!=kResultOk&&f.c->preferredZoom()==1.5,"Z02 multi-view recall not rejected");
 check(f.frames[0].calls+f.frames[1].calls==calls&&
   IntegrationAccess::view(*f.c,0)->zoomMatches(1.5)&&IntegrationAccess::view(*f.c,1)->zoomMatches(1.5),"Z02 partial multi-view change");
 evidence("Z02 PASS: attached 100/150, DPI 100/150/200, host refusals, saved reopen and atomic two-view rejection");
}
// Real VSTGUI offscreen render, exported as top-down 24-bit Windows BMP.
// The QA artifact provides pixels (including native Windows font rendering),
// not an invented screenshot or a simulation of the CSS layout.
void captureVstgui(DrumatorView& v,const std::string& title,double scale){
 const auto rect=v.getViewSize();
 check(rect.getWidth()>0&&rect.getHeight()>0,"invalid native GUI dimensions");
 auto bitmap=VSTGUI::renderBitmapOffscreen(
     VSTGUI::CPoint(rect.getWidth(),rect.getHeight()),scale,
     [&](VSTGUI::CDrawContext& ctx){v.draw(&ctx);});
 check(bitmap!=nullptr,"VSTGUI offscreen bitmap creation failed");
 auto* access=VSTGUI::CBitmapPixelAccess::create(bitmap.get(),false);
 check(access!=nullptr,"VSTGUI native bitmap pixel read failed");
 const auto width=access->getBitmapWidth(),height=access->getBitmapHeight();
 check(width>0&&height>0&&width<=4096&&height<=4096,"invalid VSTGUI screenshot pixel dimensions");
 if(title=="v2-song-board"){
   // Regression: the Song overlay must erase the entire row UI, INCLUDING
   // the 28-pixel strip to the right of the former 1252px background.
   check(width==1280&&height>200&&access->setPosition(1260,UiCompactLayout::header+32.),
         "Song overlay offscreen oracle pixel missing");
   VSTGUI::CColor pixel{};access->getColor(pixel);
   check(pixel.red==26&&pixel.green==35&&pixel.blue==45,
         "Song board leaked the underlying VELO button on its right edge");
 }

 const std::size_t stride=(static_cast<std::size_t>(width)*3u+3u)&~std::size_t{3u};
 std::array<unsigned char,54> hdr{};
 auto le16=[&](std::size_t at,std::uint16_t v){
  hdr[at]=static_cast<unsigned char>(v);hdr[at+1]=static_cast<unsigned char>(v>>8);
 };
 auto le32=[&](std::size_t at,std::uint32_t v){
  for(int b=0;b<4;++b)hdr[at+static_cast<std::size_t>(b)]=static_cast<unsigned char>(v>>(8*b));
 };
 hdr[0]='B';hdr[1]='M';
 le32(2,static_cast<std::uint32_t>(54u+stride*height));le32(10,54);
 le32(14,40);le32(18,width);
 le32(22,static_cast<std::uint32_t>(-static_cast<std::int32_t>(height)));
 le16(26,1);le16(28,24);le32(34,static_cast<std::uint32_t>(stride*height));
 std::filesystem::create_directories("gui-shots");
 std::ofstream output("gui-shots/"+title+".bmp",std::ios::binary);
 check(output.good(),"could not write native GUI screenshot");
 output.write(reinterpret_cast<const char*>(hdr.data()),hdr.size());
 std::vector<unsigned char> row(stride,0);
 std::uint64_t checksum=0;
 for(std::uint32_t y=0;y<height;++y){
  for(std::uint32_t x=0;x<width;++x){
    check(access->setPosition(x,y),"screenshot bitmap out of bounds");
    VSTGUI::CColor color{};access->getColor(color);
    const auto at=static_cast<std::size_t>(x)*3u;
    row[at]=color.blue;row[at+1]=color.green;row[at+2]=color.red;
    checksum+=color.red+color.green+color.blue;
  }
  output.write(reinterpret_cast<const char*>(row.data()),
               static_cast<std::streamsize>(row.size()));
 }
 access->forget();
 check(output.good()&&checksum>0,"blank or incomplete VSTGUI screenshot");
 evidence("Native VSTGUI render "+title+" "+std::to_string(width)+"x"+
          std::to_string(height)+" pixel checksum "+std::to_string(checksum));
}
void testGuiPhysicalStepsRaster(){
 Fixture f;f.kit(namedKit('X',4));f.open();
 auto* v=IntegrationAccess::view(*f.c,0);
 auto sample=[](DrumatorView& view,int step){
  auto rect=view.getViewSize();
  auto bmp=VSTGUI::renderBitmapOffscreen(
   VSTGUI::CPoint(rect.getWidth(),rect.getHeight()),1.,
   [&](VSTGUI::CDrawContext& dc){view.draw(&dc);});
  check(bmp!=nullptr,"GUI physical-key bitmap missing");
  auto* a=VSTGUI::CBitmapPixelAccess::create(bmp.get(),false);
  check(a!=nullptr,"GUI physical-key pixel access missing");
  std::array<std::uint32_t,17*34> pixels{};
  std::size_t k=0;
  const int left=623+19*step;
  for(int yy=static_cast<int>(UiCompactLayout::header)+69;yy<static_cast<int>(UiCompactLayout::header)+103;++yy)for(int xx=left;xx<left+17;++xx){
   check(a->setPosition(xx,yy),"GUI key raster out of view");
   VSTGUI::CColor c{};a->getColor(c);
   pixels[k++]=(std::uint32_t(c.red)<<16)|(std::uint32_t(c.green)<<8)|c.blue;
  }
  a->forget();return pixels;
 };
 // Test normal keys, quarter markers, and final key. A newly enabled
 // step gets velocity 100, not any historical value.
 for(const int step:{0,4,15,31}){
  const double x=631.+19.*step;
  if(f.c->velocity(f.c->currentPattern(),0,step)!=0)
     click(*v,x,UiCompactLayout::header+88.);
  check(f.c->velocity(f.c->currentPattern(),0,step)==0,"GUI blank baseline");
  const auto before=sample(*v,step);
  click(*v,x,UiCompactLayout::header+88.);
  check(f.c->velocity(f.c->currentPattern(),0,step)==100,
        "GUI newly activated step must have velocity 100");
  const auto after=sample(*v,step);
  std::size_t diff=0;
  for(std::size_t i=0;i<before.size();++i)if(before[i]!=after[i])++diff;
  check(diff>=65,"GUI physical keycap did not visually respond to toggle");
  click(*v,x,UiCompactLayout::header+88.);
  check(f.c->velocity(f.c->currentPattern(),0,step)==0,"GUI step did not clear");
  check(sample(*v,step)==before,"GUI keycap raster did not restore blank appearance");
 }
 evidence("GUI keycaps 0/4/15/31 active/inactive/revert native raster PASS");
}
void testWaveEditorDesignCaptures(){
 // Test fixture contains genuine 48kHz IEEE float32 stereo WAVE bytes,
 // imported through the normal encoded kit/controller/processor state.
 for(const double factor:{1.0,1.3}){
   Fixture f;
   auto kit=namedKit('X',4);
   kit.names[0]="Wave_Editor_Demo_48k_stereo.wav";
   kit.wav[0]=constantStereo(0.f,0.f);
   // Retain valid RIFF/WAVE headers, replace PCM with a transient-rich,
   // amplitude-modulated real stereo signal (not a painted waveform).
   for(std::size_t n=0;n<1024;++n){
     const double t=static_cast<double>(n)/1024.;
     const float envelope=static_cast<float>((0.15+0.75*std::abs(std::sin(t*11.0)))*
         (0.35+0.65*std::abs(std::sin(t*73.0))));
     const float l=envelope*static_cast<float>(std::sin(t*91.0));
     const float r=envelope*static_cast<float>(std::sin(t*67.0+0.6));
     std::memcpy(kit.wav[0].data()+44+n*8,&l,4);
     std::memcpy(kit.wav[0].data()+48+n*8,&r,4);
   }
   kit.sampleEdits[0]=SampleEdit{90,940,110,140,1.25f};
   f.kit(kit);
   f.open();
   auto* v=IntegrationAccess::view(*f.c,0);
   const std::uint8_t zoomCode=factor==1.0?0:2;
   std::array<std::uint8_t,5> zoomState{{'D','Z','M','2',zoomCode}};
   drumator::qa::ByteStream input(zoomState.data(),zoomState.size());
   check(f.c->setState(&input)==kResultOk&&v->zoomMatches(factor),
         "Wave editor native capture zoom mismatch");
   check(IntegrationAccess::openSampleEditorForCapture(*v,0)&&
         IntegrationAccess::sampleEditorOpen(*v),
         "Wave editor did not open loaded WAV sample");
   const auto draft=IntegrationAccess::sampleEditorDraft(*v);
   check(draft.start==90&&draft.end==940&&draft.fadeIn==110&&
         draft.fadeOut==140&&std::abs(draft.gain-1.25f)<0.001f,
         "Wave editor visible marker/fade/gain state differs from WAV test fixture");
   captureVstgui(*v,factor==1.0?"wave-editor-100":"wave-editor-130",factor);
 }
 evidence("Wave editor native captures 100/130: actual encoded stereo float32 WAV, start/end, fade-in/out and gain visible");
}
void testWaveEditor11FFlacAndStatus(){
 // Real libFLAC encoded stereo, 48kHz 16-bit, 4096 frames:
 // L=16384/32768, R=-8192/32768, independently verified source PCM.
 const std::uint8_t encodedFlac[]={0x66,0x4c,0x61,0x43,0x00,0x00,0x00,0x22,0x10,0x00,0x10,0x00,0x00,0x00,0x0e,0x00,0x00,0x0e,0x0b,0xb8,0x02,0xf0,0x00,0x00,0x10,0x00,0xa5,0x1e,0x81,0x86,0x53,0x2b,0x44,0x89,0x3f,0x27,0x7f,0x81,0x57,0xea,0x57,0xdc,0x84,0x00,0x00,0x28,0x20,0x00,0x00,0x00,0x72,0x65,0x66,0x65,0x72,0x65,0x6e,0x63,0x65,0x20,0x6c,0x69,0x62,0x46,0x4c,0x41,0x43,0x20,0x31,0x2e,0x35,0x2e,0x30,0x20,0x32,0x30,0x32,0x35,0x30,0x32,0x31,0x31,0x00,0x00,0x00,0x00,0xff,0xf8,0xca,0x18,0x00,0x7f,0x01,0x00,0x05,0x01,0x00,0x0f,0x92,0x34};
 const auto folder=std::filesystem::current_path()/"125A-W11F-import";
 std::filesystem::create_directories(folder);
 const auto path=folder/"stereo-reference.flac";
 const auto bad=folder/"broken.flac";
 const auto wav=folder/"stereo-reference.wav";
 auto write=[&](const std::filesystem::path& dest,const std::uint8_t* data,std::size_t n){
   std::ofstream file(dest,std::ios::binary);
   file.write(reinterpret_cast<const char*>(data),static_cast<std::streamsize>(n));
   check(file.good(),"W11F could not create real import fixture");
 };
 write(path,encodedFlac,sizeof(encodedFlac));
 // Independent reference check: this exact embedded FLAC passes
 // 'flac -t' with libFLAC 1.5.0 (4096 x 48kHz, 16-bit stereo).
 // Exercise converter separately before Controller/View, to locate failures.
 std::vector<std::uint8_t> converted;
 check(flacToWav(encodedFlac,sizeof(encodedFlac),converted),
       "W11G standalone dr_flac conversion failed on reference-validated FLAC");
 DecodedWav decoded;
 check(WavPcm16Decoder::decode(converted.data(),converted.size(),decoded)==WavError::Ok,
       "W11G converted float WAV rejected by existing decoder");
 check(decoded.sampleRate==48000&&decoded.channels==2&&decoded.left.size()==4096&&
       decoded.right.size()==4096,"W11G FLAC metadata/frame count incorrect");
 for(std::size_t i=0;i<4096;++i)
   check(std::abs(decoded.left[i]-.5f)<1e-6f&&
         std::abs(decoded.right[i]+.25f)<1e-6f,"W11G FLAC PCM independent channel reference");

 const std::uint8_t corrupted[]{'f','L','a','C',0,0,0,0};
 write(bad,corrupted,sizeof(corrupted));
 const auto wavBytes=constantStereo(.5f,-.25f);
 write(wav,wavBytes.data(),wavBytes.size());
 Fixture f;f.kit(namedKit('X',4));prepareAudio(*f.p);f.open();
 auto* view=IntegrationAccess::view(*f.c,0);
 // Use actual View loadPath -> Controller::loadWav -> Processor, not kit recall.
 for(const auto& imported:{wav,path}){
   if(!IntegrationAccess::importPathViaView(*view,0,pathToUtf8(imported)))
     throw std::runtime_error(std::string("W11F real import rejected: ")+
       imported.filename().string()+" / "+IntegrationAccess::sampleEditorStatus(*view));
   check(IntegrationAccess::sampleEditorStatus(*view).find("SAMPLE LOADED")!=std::string::npos,
         "W11F normal import status missing");
   check(IntegrationAccess::openSampleEditorForCapture(*view,0),
         "W11F imported sample editor could not open");
   check(IntegrationAccess::sampleEditorStatus(*view)=="SAMPLE READY FOR EDITING",
         "W11F imported editor displayed stale status");
   check(IntegrationAccess::editorFrameCount(*view)==(imported==wav?1024u:4096u)&&
         IntegrationAccess::editorStereo(*view),"W11F imported sample length/stereo");
   for(std::size_t bin=0;bin<256;++bin){
     const auto l=IntegrationAccess::editorLeft(*view)[bin];
     const auto r=IntegrationAccess::editorRight(*view)[bin];
     check(std::abs(l.minimum-.5f)<1e-4f&&std::abs(l.maximum-.5f)<1e-4f&&
           std::abs(r.minimum+.25f)<1e-4f&&std::abs(r.maximum+.25f)<1e-4f,
           "W11F decoded stereo waveform differs from independent PCM reference");
   }
   click(*view,875,473);
   check(!IntegrationAccess::sampleEditorOpen(*view),"W11F close editor after real import");
 }
 // Rejected import must preserve the previously accepted stereo sample.
 check(!IntegrationAccess::importPathViaView(*view,0,pathToUtf8(bad)),
       "W11G corrupt FLAC erroneously replaced loaded sample");
 check(IntegrationAccess::openSampleEditorForCapture(*view,0),
       "W11G corrupt FLAC damaged previous successful WAV/FLAC sample");
 check(IntegrationAccess::editorFrameCount(*view)==4096&&
       IntegrationAccess::editorStereo(*view),"W11G previous asset changed after failed FLAC");
 click(*view,875,473);
 const auto truncated=folder/"truncated.flac";
 write(truncated,encodedFlac,sizeof(encodedFlac)-8);
 check(!IntegrationAccess::importPathViaView(*view,0,pathToUtf8(truncated)),
       "W11G truncated FLAC erroneously accepted");
 check(!IntegrationAccess::importPathViaView(*view,1,pathToUtf8(bad)),
       "W11F corrupt FLAC accepted");
 check(IntegrationAccess::sampleEditorStatus(*view).find("SAMPLE LOADED")==std::string::npos,
       "W11F failed FLAC import falsely announced success");
 check(!IntegrationAccess::openSampleEditorForCapture(*view,1),
       "W11F failed FLAC imported audio in empty row");
 evidence("W11F real filesystem WAV (1024 frames) + libFLAC (4096 frames) stereo import, 256x2 PCM envelope bins, normal View status and corrupt FLAC rejection PASS");
}
void testWaveEditor11FScrollNumeric(){
 constexpr std::size_t frames=4096;
 Fixture f;auto kit=namedKit('X',4);
 auto& bytes=kit.wav[0];bytes.resize(44+frames*8);
 auto put=[&](std::size_t at,std::uint32_t v,unsigned n){
   for(unsigned k=0;k<n;++k)bytes[at+k]=static_cast<std::uint8_t>(v>>(8*k));
 };
 std::memcpy(bytes.data(),"RIFF",4);put(4,static_cast<std::uint32_t>(bytes.size()-8),4);
 std::memcpy(bytes.data()+8,"WAVEfmt ",8);
 put(16,16,4);put(20,3,2);put(22,2,2);put(24,48000,4);
 put(28,48000*8,4);put(32,8,2);put(34,32,2);
 std::memcpy(bytes.data()+36,"data",4);put(40,frames*8,4);
 std::array<float,frames> left{},right{};
 for(std::size_t i=0;i<frames;++i){
   // Unique channel- and position-dependent values; extrema are independently
   // calculated from the PCM source rather than Controller's cached envelopes.
   left[i]=static_cast<float>((i%223)/223.0-.5);
   right[i]=static_cast<float>((i%137)/137.0-.65);
   std::memcpy(bytes.data()+44+i*8,&left[i],4);
   std::memcpy(bytes.data()+48+i*8,&right[i],4);
 }
 f.kit(kit);f.open();auto* view=IntegrationAccess::view(*f.c,0);
 check(IntegrationAccess::openSampleEditorForCapture(*view,0),"W11F scroll editor open");
 auto validate=[&](std::uint32_t expectedFirst,std::uint32_t expectedLast){
   const auto [first,last]=IntegrationAccess::editorViewport(*view);
   check(first==expectedFirst&&last==expectedLast,
         "W11F scroll viewport frame boundaries incorrect");
   for(std::size_t bin=0;bin<256;++bin){
     const auto a=first+(static_cast<std::uint64_t>(last-first)*bin)/256;
     const auto b=first+(static_cast<std::uint64_t>(last-first)*(bin+1))/256;
     float minL=left[a],maxL=left[a],minR=right[a],maxR=right[a];
     for(std::size_t i=a+1;i<std::max<std::size_t>(a+1,b);++i){
       minL=std::min(minL,left[i]);maxL=std::max(maxL,left[i]);
       minR=std::min(minR,right[i]);maxR=std::max(maxR,right[i]);
     }
     const auto l=IntegrationAccess::editorLeft(*view)[bin];
     const auto r=IntegrationAccess::editorRight(*view)[bin];
     check(std::abs(l.minimum-minL)<1e-6f&&std::abs(l.maximum-maxL)<1e-6f&&
           std::abs(r.minimum-minR)<1e-6f&&std::abs(r.maximum-maxR)<1e-6f,
           "W11F scrolled stereo viewport extrema differ from independent source PCM");
   }
 };
 validate(0,4096);
 click(*view,300,473);
 validate(1024,3072);
 auto drag=[&](double from,double to){
   VSTGUI::CPoint p(from,395.);
   view->onMouseDown(p,VSTGUI::CButtonState(VSTGUI::kLButton));
   p.x=to;view->onMouseMoved(p,VSTGUI::CButtonState(VSTGUI::kLButton));
   view->onMouseUp(p,VSTGUI::CButtonState{});
 };
 drag(640,994);
 validate(2048,4096);
 drag(850,284);
 validate(0,2048);
 click(*view,380,473);
 validate(0,4096);
 click(*view,300,473);
 click(*view,470,473);
 validate(0,4096);
 evidence("W11F actual scroll thumb drags: FIT/ZOOM, both rail ends, 256x2 independently computed PCM bin extrema each viewport PASS");
}
void testWaveEditor11FStereoFadePcm(){
 Fixture f;auto kit=namedKit('X',2);
 kit.wav[0]=constantStereo(.5f,-.25f);
 f.kit(kit);prepareAudio(*f.p);
 const SampleEdit edit{64,192,32,32,1.f};
 check(f.c->previewSampleEdit(0,edit),"W11F stereo fade preview rejected");
 std::array<float,256> left{},right{};float* channels[]{left.data(),right.data()};
 AudioBusBuffers output{};output.numChannels=2;output.channelBuffers32=channels;
 ProcessData data{};data.numSamples=256;data.numOutputs=1;data.outputs=&output;
 data.symbolicSampleSize=kSample32;
 check(f.p->process(data)==kResultOk,"W11F stereo fade PCM process failure");
 for(unsigned n=0;n<256;++n){
   double envelope=n>=128?0.:1.;
   if(n<32)envelope*=static_cast<double>(n)/32.;
   if(n>=96&&n<128)envelope*=static_cast<double>(127-n)/32.;
   check(std::abs(left[n]-.5*envelope)<.006&&
         std::abs(right[n]+.25*envelope)<.006,
         "W11F stereo PCM differs from independently calculated START/END/fade reference");
 }
 check(f.c->stopSampleEditPreview(),"W11F stop stereo fade");
 evidence("W11F PCM independently calculated 256 frames x 2 channels: START 64 END 192 FADE-IN/OUT 32, silence boundaries PASS");
}
void testWaveEditorRealMouseControls(){
 Fixture f;auto kit=namedKit('X',4);
 kit.wav[0]=constantStereo(.5f,-.25f);
 kit.sampleEdits[0]=SampleEdit{90,940,110,140,1.25f};
 f.kit(kit);prepareAudio(*f.p);f.open();
 auto* v=IntegrationAccess::view(*f.c,0);
 check(IntegrationAccess::openSampleEditorForCapture(*v,0),"W11D open native editor");
 auto draft=[&]{return IntegrationAccess::sampleEditorDraft(*v);};
 auto drag=[&](double x,double y,double dest){
   VSTGUI::CPoint pos(x,y);
   v->onMouseDown(pos,VSTGUI::CButtonState(VSTGUI::kLButton));
   pos.x=dest;
   v->onMouseMoved(pos,VSTGUI::CButtonState(VSTGUI::kLButton));
   v->onMouseUp(pos,VSTGUI::CButtonState{});
 };
 // Full-screen sample viewport, select START and END at distinct x coordinates.
 drag(284.+710.*90./1024.,310.,284.+710.*140./1024.);
 check(draft().start>=138&&draft().start<=142,"W11D START drag frame mismatch");
 drag(284.+710.*940./1024.,310.,284.+710.*900./1024.);
 check(draft().end>=898&&draft().end<=902,"W11D END drag frame mismatch");
 // Upper fade handles must modify the underlying frame durations.
 const auto beforeIn=draft().fadeIn;
 drag(284.+710.*(draft().start+beforeIn)/1024.,182.,
      284.+710.*(draft().start+beforeIn+40)/1024.);
 check(draft().fadeIn>beforeIn,"W11D fade-in handle failed");
 const auto beforeOut=draft().fadeOut;
 drag(284.+710.*(draft().end-beforeOut)/1024.,182.,
      284.+710.*(draft().end-beforeOut-40)/1024.);
 check(draft().fadeOut>beforeOut,"W11D fade-out handle failed");
 // Zoom +, pan via actual mouse drag, zoom -, FIT.
 click(*v,300,473);
 check(IntegrationAccess::zoomSample(*v,false),"W11D zoom-out from button state");
 click(*v,300,473);
 drag(400,395,760);
 click(*v,380,473);
 click(*v,470,473);
 // Gain slider change and Ctrl-reset, then RESET toolbar.
 drag(680,421,815);
 check(draft().gain>1.f,"W11D gain drag failed");
 click(*v,740,421,VSTGUI::kLButton|VSTGUI::kControl);
 check(std::abs(draft().gain-1.f)<.001f,"W11D gain control-reset failed");
 click(*v,770,473);
 check(draft().start==0&&draft().end==1024&&draft().fadeIn==0&&
       draft().fadeOut==0&&std::abs(draft().gain-1.f)<.001f,
       "W11D RESET did not restore full sample");
 // Repeat preview/stop through actual controls; cancelled edits must not persist.
 for(int i=0;i<3;++i){click(*v,560,473);click(*v,665,473);}
 click(*v,350,310);
 check(draft().start>0,"W11D modified state before cancel");
 click(*v,875,473);
 check(!IntegrationAccess::sampleEditorOpen(*v),"W11D CANCEL did not close");
 check(IntegrationAccess::openSampleEditorForCapture(*v,0),"W11D reopen after cancel");
 check(draft().start==90&&draft().end==940,"W11D CANCEL persisted unsaved edit");
 // APPLY must persist changed Start when dialog is reopened.
 click(*v,380,310);
 const auto applyStart=draft().start;
 check(applyStart!=90,"W11D change before APPLY");
 click(*v,970,473);
 check(!IntegrationAccess::sampleEditorOpen(*v),"W11D APPLY did not close");
 check(IntegrationAccess::openSampleEditorForCapture(*v,0),"W11D reopen after apply");
 check(draft().start==applyStart,"W11D APPLY failed to persist edit");
 evidence("W11D real native VSTGUI mouse controls: marker/fade drag, zoom/scroll, gain/reset, preview/stop, apply/cancel PASS");
}
void testWaveEditorLongCaptures(){
 constexpr std::uint32_t rate=48000;
 constexpr std::size_t frames=rate*2;
 for(double factor:{1.0,1.3}){
   Fixture f;
   auto kit=namedKit('X',4);
   auto& wav=kit.wav[0];
   wav.resize(44+frames*8);
   auto put=[&](std::size_t at,std::uint32_t value,unsigned count){
     for(unsigned n=0;n<count;++n)wav[at+n]=static_cast<std::uint8_t>(value>>(8*n));
   };
   std::memcpy(wav.data(),"RIFF",4);put(4,static_cast<std::uint32_t>(wav.size()-8),4);
   std::memcpy(wav.data()+8,"WAVEfmt ",8);
   put(16,16,4);put(20,3,2);put(22,2,2);put(24,rate,4);
   put(28,rate*8,4);put(32,8,2);put(34,32,2);
   std::memcpy(wav.data()+36,"data",4);put(40,static_cast<std::uint32_t>(frames*8),4);
   for(std::size_t i=0;i<frames;++i){
     const double t=static_cast<double>(i)/rate;
     const float env=static_cast<float>((0.3+0.6*std::abs(std::sin(t*7.7)))*
                            (0.5+0.5*std::abs(std::sin(t*49.))));
     const float l=env*static_cast<float>(std::sin(t*730.));
     const float r=env*static_cast<float>(std::sin(t*920.+.4));
     std::memcpy(wav.data()+44+i*8,&l,4);
     std::memcpy(wav.data()+48+i*8,&r,4);
   }
   // Deliberately do not invent a sample name: embedded audio alone is sufficient.
   kit.names[0]="wave-qa-synthetic.wav"; // Required internal KitCodec identity; never shown as an imported filename.
   kit.sampleEdits[0]=SampleEdit{4800,86400,4800,7200,1.25f};
   f.kit(kit);f.open();
   auto* v=IntegrationAccess::view(*f.c,0);
   std::array<std::uint8_t,5> zoomState{{'D','Z','M','2',
      static_cast<std::uint8_t>(factor==1.?0:2)}};
   drumator::qa::ByteStream input(zoomState.data(),zoomState.size());
   check(f.c->setState(&input)==kResultOk&&v->zoomMatches(factor),
         "Wave long capture editor zoom");
   check(IntegrationAccess::openSampleEditorForCapture(*v,0),
         "Wave long capture loaded audio unavailable");
   check(IntegrationAccess::sampleEditorStatus(*v).find("EMPTY")==std::string::npos,
         "Wave editor incorrectly shows EMPTY for loaded audio");
   check(f.c->editorWaveSampleRate()==rate,"Wave editor source rate mismatch");
   captureVstgui(*v,factor==1.?"wave-long-fit-100":"wave-long-fit-130",factor);
   check(IntegrationAccess::zoomSample(*v,true),"Wave long zoom-in failed");
   captureVstgui(*v,factor==1.?"wave-long-zoom-100":"wave-long-zoom-130",factor);
 }
 evidence("Wave long sample: native 2s 48kHz stereo WAV, zoomed/full 100/130 and no EMPTY PASS");
}
void testVelocityDesignCaptures(){
 // Native GUI design fixture only. No production implementation changes.
 constexpr std::array<std::size_t,6> steps{{0,4,8,12,16,20}};
 constexpr std::array<std::uint8_t,6> values{{20,40,64,90,110,127}};
 for(const auto zoomFactor: {1.0,1.3}){
   Fixture f;
   auto kit=namedKit('X',4);
   kit.grid.fill(0);
   for(std::size_t n=0;n<steps.size();++n)kit.grid[steps[n]]=values[n];
   f.kit(kit);
   f.open();
   auto* v=IntegrationAccess::view(*f.c,0);
   const std::uint8_t zoomCode=zoomFactor==1.0?0:2;
   std::array<std::uint8_t,5> zoomState{{'D','Z','M','2',zoomCode}};
   drumator::qa::ByteStream input(zoomState.data(),zoomState.size());
   check(f.c->setState(&input)==kResultOk&&v->zoomMatches(zoomFactor),
         "Velocity visual QA: actual host/editor zoom mismatch");
   click(*v,1248.,UiCompactLayout::header+88.);
   check(IntegrationAccess::velocityExpanded(*v,0),
         "Velocity visual QA: first row did not expand");
   for(std::size_t n=0;n<kSteps;++n){
     std::uint8_t expected=0;
     for(std::size_t i=0;i<steps.size();++i)if(steps[i]==n)expected=values[i];
     check(f.c->velocity(0,0,n)==expected,
           "Velocity visual QA: fixture values differ from actual controller state");
   }
   captureVstgui(*v,zoomFactor==1.0?"velocity-design-100":"velocity-design-130",zoomFactor);
 }
 evidence("Velocity design reference: six native velocity levels 20/40/64/90/110/127 at real 100% and 130% editor zoom");
}
void testGui49MusicalFourSampleCapture(){
 Fixture f;KitState kit;kit.visible=4;kit.selected=0;kit.grid.fill(0);
 constexpr const char* labels[]={"KICK","SNARE","CLOSED HI-HAT","OPEN HI-HAT"};
 constexpr const char* names[]={"Kick.wav","Snare.wav","Closed_HiHat.wav","Open_HiHat.wav"};
 for(std::size_t row=0;row<4;++row){
   kit.names[row]=names[row];kit.displayNames[row]=labels[row];
   auto wav=constantStereo(0.f,0.f);
   constexpr float pi=3.14159265358979323846f;
   for(std::size_t n=0;n<1024;++n){
     const float time=static_cast<float>(n)/48000.f;
     const float decay=std::exp(-time*(row==0?110.f:row==1?170.f:row==2?260.f:85.f));
     const float freq=row==0?65.f:row==1?180.f:row==2?2200.f:1250.f;
     const float signal=decay*std::sin(2.f*pi*freq*time);
     const float l=signal*.68f,r=signal*.61f;
     std::memcpy(wav.data()+44+n*8,&l,4);
     std::memcpy(wav.data()+48+n*8,&r,4);
   }
   kit.wav[row]=std::move(wav);
 }
 // Two 4/4 bars of 16th notes. Velocity is the actual occupied-step value.
 auto set=[&](std::size_t row,std::size_t step,unsigned velocity){
    kit.grid[row*kSteps+step]=static_cast<std::uint8_t>(velocity);
 };
 for(auto step:{0u,8u,16u,24u})set(0,step,115);
 for(auto step:{4u,12u,20u,28u})set(1,step,112);
 for(unsigned step=0;step<32;step+=2)set(2,step,(step%8==6)?72:98);
 for(auto step:{14u,30u})set(3,step,86);
 kit.patternLengths[0]=32;
 f.kit(kit);prepareAudio(*f.p);f.open();
 auto* view=IntegrationAccess::view(*f.c,0);
 const auto folder=std::filesystem::current_path()/"125A-GUI49-real-import-capture";
 std::filesystem::create_directories(folder);
 for(std::size_t row=0;row<4;++row){
   const auto file=folder/names[row];
   std::ofstream stream(file,std::ios::binary|std::ios::trunc);
   stream.write(reinterpret_cast<const char*>(kit.wav[row].data()),
                static_cast<std::streamsize>(kit.wav[row].size()));
   check(stream.good(),"GUI49 fixture WAV could not be written");
   stream.close();
   check(IntegrationAccess::importPathViaView(*view,row,pathToUtf8(file)),
         "GUI49 sample not accepted through normal WAV import");
 }
 check(IntegrationAccess::sampleEditorStatus(*view).find("SAMPLE LOADED")!=std::string::npos,
       "GUI49 screenshot must show the authentic loaded-sample status");
 for(std::size_t row=0;row<4;++row){
   check(f.c->hasSample(row)&&f.c->label(row)==labels[row],
         "GUI49 musical capture must contain four actual loaded stereo samples");
 }
 for(std::size_t row=0;row<4;++row)
   for(std::size_t step=0;step<32;++step)
     check(f.c->velocity(0,row,step)==kit.grid[row*kSteps+step],
           "GUI49 musical capture actual 32-step state mismatch");
 const std::array<std::uint8_t,5> z{{'D','Z','M','2',2}};
 drumator::qa::ByteStream state(const_cast<std::uint8_t*>(z.data()),z.size());
 check(f.c->setState(&state)==kResultOk&&view->zoomMatches(1.3),
       "GUI49 musical screenshot must use real attached 130% zoom");
 captureVstgui(*view,"gui49-four-real-samples-musical-130",1.3);
 evidence("GUI49 musical native 130pct: four decoded stereo WAVs, true names, occupied 32-step velocities and attached zoom PASS");
}
void test11IMeterStereoAndStems(){
 // Independent exact buffer oracle: MAIN channel asymmetry, 0dBFS versus
 // true over-range, stem-only clipping and disabled stem guard.
 OutputMeter measured;
 std::array<float,8> left{{0.f,.125f,.5f,.99f,1.f,1.25f,.125f,0.f}};
 std::array<float,8> right{{0.f,.25f,.125f,.0625f,.5f,.0625f,0.f,0.f}};
 std::array<float,8> quiet{},quietR{},stemL{},stemR{};
 stemL.fill(1.375f);stemR.fill(.1f);
 float* mainChannels[]{left.data(),right.data()};
 float* stemChannels[]{stemL.data(),stemR.data()};
 AudioBusBuffers bus[2]{};
 bus[0].numChannels=2;bus[0].channelBuffers32=mainChannels;
 bus[1].numChannels=2;bus[1].channelBuffers32=stemChannels;
 ProcessData data{};data.numSamples=8;data.numOutputs=2;data.outputs=bus;
 measured.observe(data,0);
 check(measured.clipMask.load()==1u&&measured.fullScaleMask.load()==1u,
       "M11I MAIN exact/above-0dBFS distinctions or disabled STEM");
 check(std::abs(OutputMeter::decode(measured.mainL.exchange(0))-1.25f)<.0002f&&
       std::abs(OutputMeter::decode(measured.mainR.exchange(0))-.5f)<.0002f,
       "M11I independent stereo MAIN sample peaks");
 measured.clearClips();
 // Boundary-only MAIN exactly reaches 0 dBFS without exceeding full scale.
 std::array<float,8> thresholdL{{0.f,.2f,1.f,.4f,0.f,0.f,0.f,0.f}};
 float* thresholdChannels[]{thresholdL.data(),right.data()};
 bus[0].channelBuffers32=thresholdChannels;
 measured.observe(data,0);
 check(measured.clipMask.load()==0u&&measured.fullScaleMask.load()==1u,
       "M11I exact 0dBFS must not be classified as above-full-scale CLIP");
 measured.clearClips();(void)measured.mainL.exchange(0);(void)measured.mainR.exchange(0);
 float* silentChannels[]{quiet.data(),quietR.data()};
 bus[0].channelBuffers32=silentChannels;
 measured.observe(data,1u<<1);
 check(measured.clipMask.load()==2u&&
       measured.mainL.exchange(0)==0u&&measured.mainR.exchange(0)==0u,
       "M11I stem-only overrange must warn with silent MAIN");
 measured.clearClips();
 check(measured.clipMask.load()==0&&measured.fullScaleMask.load()==0,
       "M11I manual CLIP reset");
 bus[0].channelBuffers32=mainChannels;
 data.numSamples=0;measured.observe(data,0);
 check(measured.clipMask.load()==0,"M11I zero-frame callback changed latch");

 // Actual Processor -> post-master output PCM -> atomic meter -> Controller link.
 Fixture f;auto kit=namedKit('X',2);
 kit.wav[0]=constantStereo(.75f,-.375f);kit.masterVolume=200;
 f.kit(kit);prepareAudio(*f.p);
 auto meter=f.c->meterLink();
 check(meter!=nullptr,"M11I Processor-to-Controller meter connection missing");
 check(f.c->previewSample(0),"M11I real row preview failed");
 std::array<float,256> actualL{},actualR{};
 float* actualChannels[]{actualL.data(),actualR.data()};
 AudioBusBuffers out{};out.numChannels=2;out.channelBuffers32=actualChannels;
 ProcessData block{};block.numSamples=256;block.numOutputs=1;block.outputs=&out;
 block.symbolicSampleSize=kSample32;
 check(f.p->process(block)==kResultOk,"M11I actual audio process failed");
 float expectedL=0.f,expectedR=0.f;
 for(unsigned i=0;i<256;++i) {
   expectedL=std::max(expectedL,std::abs(actualL[i]));
   expectedR=std::max(expectedR,std::abs(actualR[i]));
 }
 check(expectedL>1.f&&expectedR>0.f,"M11I actual master gain/preview not audible");
 const float reportedL=OutputMeter::decode(meter->mainL.exchange(0));
 const float reportedR=OutputMeter::decode(meter->mainR.exchange(0));
 check(std::abs(expectedL-reportedL)<.001f&&std::abs(expectedR-reportedR)<.001f,
       "M11I meter does not match actual post-master PCM");
 check(meter->clipMask.load()&1u,"M11I actual MAIN overrange was not latched");
 meter->clearClips();
 f.open();auto* v=IntegrationAccess::view(*f.c,0);
 check(f.c->previewSample(0),"M11I second preview for native GUI meter");
 check(f.p->process(block)==kResultOk,"M11I second real master render");
 IntegrationAccess::pollMeter(*v);
 check(IntegrationAccess::meterPaintState(*v)[0]>1.f,
       "M11I GUI does not receive real MAIN peak");
 for(double zoomFactor:{1.,1.3}){
   const std::uint8_t z=zoomFactor==1.?0:2;
   std::array<std::uint8_t,5> zoomState{{'D','Z','M','2',z}};
   drumator::qa::ByteStream in(zoomState.data(),zoomState.size());
   check(f.c->setState(&in)==kResultOk&&v->zoomMatches(zoomFactor),
         "M11I native GUI zoom contract");
   captureVstgui(*v,zoomFactor==1.?"gui49-11i-main-meter-100":"gui49-11i-main-meter-130",zoomFactor);
 }
 click(*v,1264,79);
 check(meter->clipMask.load()==0,"M11I GUI left-click failed to clear clip");

 // Render a real independently routed stem with MAIN silent.
 Fixture stems;auto stemKit=namedKit('X',2);
 stemKit.wav[1]=constantStereo(.75f,-.375f);stemKit.masterVolume=200;
 stemKit.output[1]=1;
 stems.kit(stemKit);prepareAudio(*stems.p);
 check(stems.p->activateBus(MediaTypes::kAudio,BusDirections::kOutput,1,true)==kResultOk,
       "M11I stem host activation failed");
 check(stems.c->previewSample(1),"M11I stem preview rejected");
 std::array<float,256> mainL{},mainR{},outL{},outR{};
 float* mainOut[]{mainL.data(),mainR.data()};
 float* stemOut[]{outL.data(),outR.data()};
 AudioBusBuffers buses[2]{};buses[0].numChannels=2;buses[0].channelBuffers32=mainOut;
 buses[1].numChannels=2;buses[1].channelBuffers32=stemOut;
 ProcessData multi{};multi.numSamples=256;multi.numOutputs=2;
 multi.outputs=buses;multi.symbolicSampleSize=kSample32;
 check(stems.p->process(multi)==kResultOk,"M11I actual stem process failed");
 float maxStem=0.f,maxMain=0.f;
 for(unsigned i=0;i<256;++i){
   maxStem=std::max(maxStem,std::abs(outL[i]));
   maxMain=std::max(maxMain,std::abs(mainL[i]));
 }
 auto stemMeter=stems.c->meterLink();
 check(stemMeter&&maxStem>1.f&&maxMain<1e-5f&&
       (stemMeter->clipMask.load()&(1u<<1))&&
       stemMeter->mainL.exchange(0)==0,
       "M11I OUT 2-only true overrange with silent MAIN missing");
 stems.open();auto* vs=IntegrationAccess::view(*stems.c,0);
 IntegrationAccess::pollMeter(*vs);
 const std::array<std::uint8_t,5> scale{{'D','Z','M','2',2}};
 drumator::qa::ByteStream state(const_cast<std::uint8_t*>(scale.data()),scale.size());
 check(stems.c->setState(&state)==kResultOk&&vs->zoomMatches(1.3),
       "M11I stem CLIP capture 130pct");
 captureVstgui(*vs,"gui49-11i-out2-clip-130",1.3);
 evidence("M11I MAIN L/R real post-ramp samples, separate 0dBFS/overrange, 16-bus inactive mask, OUT2-only clip and click reset PASS");
}
void test11IHeaderPatternCentered(){
  // Fixed centered product title is the alignment datum for eight A-H keys.
  constexpr double patternAxis=UiCompactLayout::patternStart+
      (7.*UiCompactLayout::patternPitch+UiCompactLayout::patternButtonWidth)/2.;
  check(patternAxis==UiCompactLayout::width/2.,
        "GUI 11I PATTERN A-H group not centered below DRUMATOR");
  check(UiCompactLayout::barTwo.right<UiCompactLayout::patternStart&&
        UiCompactLayout::patternStart+7.*UiCompactLayout::patternPitch+
          UiCompactLayout::patternButtonWidth<UiCompactLayout::patternTools.left&&
        UiCompactLayout::patternTools.right<UiCompactLayout::songToggle.left&&
        UiCompactLayout::songToggle.right<UiCompactLayout::songEdit.left&&
        UiCompactLayout::songEdit.right<UiCompactLayout::globalKnobCenters[2]-50.,
        "GUI 11I moved header controls collide");
  check(UiCompactLayout::meterRight+7.<=UiCompactLayout::width-12.&&
        UiCompactLayout::meterClipRight<=UiCompactLayout::width-12.&&
        UiCompactLayout::meterLeft>UiCompactLayout::globalKnobCenters[2]+72.,
        "GUI 11I meter clips chassis or MASTER readout");
  check(UiCompactLayout::meterResetLeft<UiCompactLayout::meterLeft&&
        UiCompactLayout::meterResetRight>UiCompactLayout::meterRight+7.,
        "GUI 11I new meter reset click zone misses LED columns");
  evidence("GUI 11I centered PATTERN A-H, following groups, MASTER and meter margins PASS");
}
void test11IRotaryMinCenterMax(){
 struct ControlState {
   const char* name;
   std::uint8_t swing,humanize,master,volume,pan,pitch;
 };
 constexpr ControlState cases[]{
   {"minimum",0,0,0,0,0,0},
   {"center",25,50,100,100,100,24},
   {"maximum",50,100,200,200,200,48}
 };
 for(const auto& state:cases){
   Fixture f;auto kit=namedKit('X',4);
   kit.swing=state.swing;kit.humanize=state.humanize;
   kit.masterVolume=state.master;kit.gain[0]=state.volume;
   kit.pan[0]=state.pan;kit.pitch[0]=state.pitch;
   f.kit(kit);f.open();
   auto* v=IntegrationAccess::view(*f.c,0);
   check(f.c->swingPercent()==state.swing&&
         f.c->humanizePercent()==state.humanize&&
         f.c->masterVolume()==state.master&&
         f.c->rowGain(0)==state.volume&&
         f.c->rowPan(0)==static_cast<int>(state.pan)-100&&
         f.c->rowPitch(0)==static_cast<int>(state.pitch)-24,
         "M11I rotary numeric min/center/max does not match kit/controller state");
   for(double factor:{1.,1.3}){
     const std::uint8_t z=factor==1.?0:2;
     std::array<std::uint8_t,5> zoomState{{'D','Z','M','2',z}};
     drumator::qa::ByteStream in(zoomState.data(),zoomState.size());
     check(f.c->setState(&in)==kResultOk&&v->zoomMatches(factor),
           "M11I rotary actual native zoom invalid");
     captureVstgui(*v,std::string("gui49-11i-rotary-")+state.name+
                     (factor==1.?"-100":"-130"),factor);
   }
 }
 Fixture f;auto kit=namedKit('X',4);
 kit.swing=40;kit.humanize=80;kit.masterVolume=175;
 kit.gain[0]=180;kit.pan[0]=170;kit.pitch[0]=41;
 f.kit(kit);prepareAudio(*f.p);f.open();auto* v=IntegrationAccess::view(*f.c,0);
 for(const auto x:{UiCompactLayout::globalKnobCenters[0],
                    UiCompactLayout::globalKnobCenters[1],
                    UiCompactLayout::globalKnobCenters[2]})
   click(*v,x,UiCompactLayout::globalKnobY,VSTGUI::kLButton|VSTGUI::kDoubleClick);
 const auto rowY=UiCompactLayout::header+UiCompactLayout::rowKnobY;
 for(std::size_t i=0;i<3;++i)
   click(*v,UiCompactLayout::rowKnobCenters[i],rowY,
         VSTGUI::kLButton|VSTGUI::kDoubleClick);
 const auto resetDiagnostic=std::string("SWING=")+std::to_string(f.c->swingPercent())+
   " HUMANIZE="+std::to_string(f.c->humanizePercent())+
   " MASTER="+std::to_string(f.c->masterVolume())+
   " VOL="+std::to_string(f.c->rowGain(0))+
   " PAN="+std::to_string(f.c->rowPan(0))+
   " PITCH="+std::to_string(f.c->rowPitch(0));
 check(f.c->swingPercent()==0&&f.c->humanizePercent()==0&&
       f.c->masterVolume()==100&&f.c->rowGain(0)==100&&
       f.c->rowPan(0)==0&&f.c->rowPitch(0)==0,
       (std::string("M11I double-click reset values: ")+resetDiagnostic).c_str());
 evidence("M11I six rotary types min/center/max, exact numeric state, real native 100/130 screenshots, six double-click defaults PASS");
}
void test11IMonoFlacImport(){
 static constexpr std::uint8_t monoFlac[]={0x66,0x4c,0x61,0x43,0x00,0x00,0x00,0x22,0x10,0x00,0x10,0x00,0x00,0x00,0x0b,0x00,0x00,0x0b,0x0b,0xb8,0x00,0xf0,0x00,0x00,0x04,0x00,0xba,0x31,0x49,0x4f,0xa5,0x46,0x04,0x02,0x5c,0x12,0x84,0x74,0xa9,0x79,0x53,0xe4,0x84,0x00,0x00,0x28,0x20,0x00,0x00,0x00,0x72,0x65,0x66,0x65,0x72,0x65,0x6e,0x63,0x65,0x20,0x6c,0x69,0x62,0x46,0x4c,0x41,0x43,0x20,0x31,0x2e,0x35,0x2e,0x30,0x20,0x32,0x30,0x32,0x35,0x30,0x32,0x31,0x31,0x00,0x00,0x00,0x00,0xff,0xf8,0xaa,0x08,0x00,0xed,0x01,0x00,0x09,0xa4,0xb8};
 // The exact 97-byte file passes independent libFLAC 1.5.0 'flac -t'.
 const auto path=std::filesystem::current_path()/"125A-11I-mono.flac";
 {std::ofstream file(path,std::ios::binary|std::ios::trunc);
  file.write(reinterpret_cast<const char*>(monoFlac),sizeof(monoFlac));
  check(file.good(),"M11I failed to create Mono FLAC fixture");}
 Fixture f;f.kit(namedKit('X',2));prepareAudio(*f.p);f.open();
 auto* view=IntegrationAccess::view(*f.c,0);
 check(IntegrationAccess::importPathViaView(*view,0,pathToUtf8(path)),
       "M11I normal mono-FLAC file import rejected");
 check(IntegrationAccess::openSampleEditorForCapture(*view,0),
       "M11I mono-FLAC editor open failed");
 check(IntegrationAccess::editorFrameCount(*view)==1024&&
       !IntegrationAccess::editorStereo(*view),"M11I mono-FLAC channels/frames incorrect");
 for(const auto& v:IntegrationAccess::editorLeft(*view))
   check(std::abs(v.minimum-.25f)<1e-6f&&std::abs(v.maximum-.25f)<1e-6f,
         "M11I decoded mono-FLAC PCM level incorrect");
 evidence("M11I libFLAC validated mono 48kHz/16bit/1024 frames, 256 PCM waveform bins, normal import and editor PASS");
}
void testVisualRender(){
 for(const auto zoom: {1.0,1.2,1.3,1.5}){
   Fixture f;auto kit=namedKit('X',4);
   kit.displayNames[0]="KICK_LONG_AUDIO_SAMPLE_NAME_IN_2026.wav";
   f.kit(kit);f.open();
   auto* v=IntegrationAccess::view(*f.c,0);
   // Raster scaling alone previously produced a misleading 150%-named
   // screenshot whose actual plugin zoom indicator still read 100%.
   // Exercise the real attached editor zoom before taking each native render.
   const std::uint8_t zoomCode=zoom==1.?0:zoom==1.2?1:zoom==1.3?2:3;
   std::array<std::uint8_t,5> zoomState{{'D','Z','M','2',zoomCode}};
   drumator::qa::ByteStream zoomInput(zoomState.data(),zoomState.size());
   check(f.c->setState(&zoomInput)==kResultOk&&v->zoomMatches(zoom),
         "native screenshot zoom state does not match raster scale");
   captureVstgui(*v,zoom==1.?"v2-four-rows-100":zoom==1.2?"v2-four-rows-120":
        zoom==1.3?"v2-four-rows-130":"v2-four-rows-150",zoom);
 }
 {
   Fixture f;f.kit(namedKit('X',16));f.open();
   auto* v=IntegrationAccess::view(*f.c,0);
   captureVstgui(*v,"v2-16-rows-page-1",1.0);
   // GUI46: inspect bottom fascia and group navigation with 16 instruments
   // at actual host zoom (not merely an upscaled raster).
   std::array<std::uint8_t,5> footerZoom{{'D','Z','M','2',2}};
   drumator::qa::ByteStream footerZoomInput(footerZoom.data(),footerZoom.size());
   check(f.c->setState(&footerZoomInput)==kResultOk&&v->zoomMatches(1.3),
         "GUI46 native 16-row footer capture must use true 130pct zoom");
   captureVstgui(*v,"gui46-16-rows-130",1.3);
   // GUI47: actual 5-8 navigation state at 130%; footer counter,
   // viewport and vertical scrollbar all read the same firstRow_.
   const auto bottomPage1=v->getViewSize().getHeight()-UiCompactLayout::footer;
   click(*v,1190.,bottomPage1+17.);
   check(IntegrationAccess::firstRow(*v)==4&&IntegrationAccess::selectedRow(*v)==4,
         "GUI47 right footer arrow did not select instrument group 5-8");
   captureVstgui(*v,"gui47-instruments-5-8-130",1.3);
   const auto bottomPage2=v->getViewSize().getHeight()-UiCompactLayout::footer;
   click(*v,1100.,bottomPage2+17.);
   check(IntegrationAccess::firstRow(*v)==0,
         "GUI47 left footer arrow did not return to instruments 1-4");

   std::array<std::uint8_t,5> defaultZoom{{'D','Z','M','2',0}};
   drumator::qa::ByteStream defaultZoomInput(defaultZoom.data(),defaultZoom.size());
   check(f.c->setState(&defaultZoomInput)==kResultOk&&v->zoomMatches(1.0),
         "GUI46 reset editor zoom after footer capture");
   click(*v,900.,114.);
   captureVstgui(*v,"v2-pattern-actions-menu",1.0);
   click(*v,15.,390.);
   click(*v,1150.,20.);
   captureVstgui(*v,"v2-zoom-menu",1.0);
   click(*v,15.,390.);
   click(*v,1248,UiCompactLayout::header+88.);
   check(IntegrationAccess::velocityExpanded(*v,0),"V2 row 1 expansion failed");
   const auto row2=IntegrationAccess::rowTop(*v,1);
   click(*v,1248,row2+88.);
   check(IntegrationAccess::velocityExpanded(*v,0)&&
         IntegrationAccess::velocityExpanded(*v,1),"V2 expansion changed another row");
   captureVstgui(*v,"v2-two-velocity-rows",1.0);
   click(*v,289.,UiCompactLayout::header+19.); // explicit SAMPLE actions menu
   captureVstgui(*v,"v2-sample-actions-menu",1.0);
   click(*v,15.,400.); // outside dismisses without triggering a step
   click(*v,575.,UiCompactLayout::header+19.); // MAIN / OUT selector
   captureVstgui(*v,"v2-output-menu",1.0);
   click(*v,15.,400.);
   // The CHOKE label is at rowTop+52..73, but the clickable selector is
   // rowTop+73..105. The former +69 capture silently recorded a CLOSED
   // popup identical to v2-two-velocity-rows; reject that false visual PASS.
   click(*v,592.,UiCompactLayout::header+88.); // center of actual CHOKE selector
   check(IntegrationAccess::popup(*v)==4&&IntegrationAccess::popupRow(*v)==0,
         "V2 choke visual oracle: menu did not open from actual selector");
   captureVstgui(*v,"v2-choke-menu",1.0);
   click(*v,15.,400.);
   check(IntegrationAccess::popup(*v)==0,"V2 choke popup did not dismiss");
   click(*v,425.,UiCompactLayout::header+19.); // row-local Fill menu
   captureVstgui(*v,"v2-fill-menu",1.0);
   click(*v,15.,400.);
   for(int i=0;i<2;++i){
     const auto bottom=v->getViewSize().getHeight()-UiCompactLayout::footer;
     click(*v,1190,bottom+17.);
   }
   check(IntegrationAccess::firstRow(*v)==8&&IntegrationAccess::selectedRow(*v)==8,
         "V2 third page selection/paging failed");
   captureVstgui(*v,"v2-16-rows-page-3",1.0);
   // Scrollbar must reach the last four actual instruments by thumb drag.
   click(*v,1271.,UiCompactLayout::scrollTop+1.);
   check(IntegrationAccess::firstRow(*v)==0,"scrollbar top");
   const double rail=IntegrationAccess::rowTop(*v,4)-8.-UiCompactLayout::scrollTop;
   VSTGUI::CPoint thumb(1271.,UiCompactLayout::scrollTop+rail/8.);
   v->onMouseDown(thumb,VSTGUI::CButtonState(VSTGUI::kLButton));
   thumb.y=UiCompactLayout::scrollTop+rail-2.;
   v->onMouseMoved(thumb,VSTGUI::CButtonState(VSTGUI::kLButton));
   v->onMouseUp(thumb,{});
   check(IntegrationAccess::firstRow(*v)==12,"scrollbar last page");
   captureVstgui(*v,"v2-16-rows-scrolled",1.0);
 }
 {
   Fixture f;f.kit(namedKit('X',4));f.open();
   auto* v=IntegrationAccess::view(*f.c,0);
   click(*v,1077,114.);
   captureVstgui(*v,"v2-song-board",1.0);
 }
 {
   // GUI39: actual editor state and native captured inactive/active indicators.
   Fixture f;f.kit(namedKit('X',4));f.open();
   auto* v=IntegrationAccess::view(*f.c,0);
   constexpr double zoom=1.3;
   std::array<std::uint8_t,5> zoomState{{'D','Z','M','2',2}};
   drumator::qa::ByteStream zoomInput(zoomState.data(),zoomState.size());
   check(f.c->setState(&zoomInput)==kResultOk&&v->zoomMatches(zoom),
         "GUI39 status raster requires actual 130pct editor zoom");
   captureVstgui(*v,"gui39-inactive-130",zoom);
   const double top=UiCompactLayout::header;
   click(*v,473.,top+22.); // MUTE row 0
   click(*v,513.,top+22.); // SOLO row 0
   click(*v,1246.,top+88.); // VEL row 0
   check(f.c->isRowMuted(0)&&f.c->isRowSolo(0)&&
         IntegrationAccess::velocityExpanded(*v,0),
         "GUI39 active lamps not backed by real controller/view states");
   captureVstgui(*v,"gui39-active-130",zoom);
 }
 evidence("V2 native VSTGUI vector controls and per-row Velocity windows captured");
}
void testPrecisionPanel(){
 Fixture f;f.kit(namedKit('X'));
 ProcessSetup setup{};
 setup.sampleRate=48000.;setup.maxSamplesPerBlock=256;
 setup.symbolicSampleSize=kSample32;setup.processMode=kRealtime;
 check(f.p->setupProcessing(setup)==kResultOk,"V2 pitch rate not initialized");
 f.open();auto* v=IntegrationAccess::view(*f.c,0);
 auto drag=[&](double x,double y,double dy){
   click(*v,x,y);
   VSTGUI::CPoint at(x,y+dy);
   v->onMouseMoved(at,VSTGUI::CButtonState(VSTGUI::kLButton));
   v->onMouseUp(at,{});
 };
 const double top=UiCompactLayout::header;
 drag(133.,top+UiCompactLayout::rowKnobY,-25.);
 check(f.c->rowGain(0)==117,"UX rotary gain 25px must increase 17%");
 click(*v,133.,top+UiCompactLayout::rowKnobY,VSTGUI::kLButton|VSTGUI::kControl);
 check(f.c->rowGain(0)==100,"V2 rotary Ctrl default not 100");
 {
   VSTGUI::CPoint pt(133.,top+UiCompactLayout::rowKnobY);
   v->onMouseDown(pt,VSTGUI::CButtonState(VSTGUI::kLButton|VSTGUI::kShift));
   pt.y-=15.;
   v->onMouseMoved(pt,VSTGUI::CButtonState(VSTGUI::kLButton));
   v->onMouseUp(pt,{});
   check(f.c->rowGain(0)==103,"UX fine knob sensitivity not latched");
   click(*v,133.,top+UiCompactLayout::rowKnobY,VSTGUI::kLButton|VSTGUI::kControl);
   check(f.c->rowGain(0)==100,"UX fine knob Ctrl reset failed");
 }
 // P11: actual VSTGUI PAN +100 must be reachable AND reversible;
 // regress the original owner report where a fully right-panned knob stuck.
 drag(254.,top+UiCompactLayout::rowKnobY,-150.);
 check(f.c->rowPan(0)==100,"P11 PAN +100 rotary endpoint unreachable");
 {
   ResizableMemoryIBStream saved;KitState state;
   check(f.p->getState(&saved)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(saved.getData()),
             static_cast<std::size_t>(saved.getCursor()),state)&&state.pan[0]==200,
         "P11 +100 panorama not persisted by actual processor");
 }
 drag(254.,top+UiCompactLayout::rowKnobY,30.);
 check(f.c->rowPan(0)==80,"P11 PAN jammed at +100 on reverse drag");
 drag(254.,top+UiCompactLayout::rowKnobY,300.);
 check(f.c->rowPan(0)==-100,"P11 PAN -100 endpoint unreachable");
 {
   ResizableMemoryIBStream saved;KitState state;
   check(f.p->getState(&saved)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(saved.getData()),
             static_cast<std::size_t>(saved.getCursor()),state)&&state.pan[0]==0,
         "P11 -100 panorama not persisted by actual processor");
 }
 click(*v,254.,top+UiCompactLayout::rowKnobY,VSTGUI::kLButton|VSTGUI::kControl);
 check(f.c->rowPan(0)==0,"P11 PAN Ctrl reset to center failed");
 // Volume must also leave its maximum, and default must stay at 100%.
 drag(133.,top+UiCompactLayout::rowKnobY,-150.);
 check(f.c->rowGain(0)==200,"P11 VOL 200% endpoint unreachable");
 drag(133.,top+UiCompactLayout::rowKnobY,30.);
 check(f.c->rowGain(0)==180,"P11 VOL stuck at 200% after reversing");
 click(*v,133.,top+UiCompactLayout::rowKnobY,VSTGUI::kLButton|VSTGUI::kControl);
 check(f.c->rowGain(0)==100,"P11 VOL Ctrl reset not 100%");
 drag(375.,top+UiCompactLayout::rowKnobY,-60.);
 check(f.c->rowPitch(0)==24,"V2 pitch rotary maximum failed");
 drag(375.,top+UiCompactLayout::rowKnobY,30.);
 check(f.c->rowPitch(0)==12,"P11 PITCH stuck at +24 semitones");
 click(*v,375.,top+UiCompactLayout::rowKnobY,VSTGUI::kLButton|VSTGUI::kControl);
 check(f.c->rowPitch(0)==0,"P11 PITCH Ctrl reset failed");
 drag(496.,top+UiCompactLayout::rowKnobY,-250.);
 check(f.c->rowRandomVelocity(0)==0,"V2 button unexpectedly enabled legacy realtime random");
 // Existing projects with stored nonzero RT retain their usable old control.
 {
  Fixture legacy;
  auto oldKit=namedKit('X');
  oldKit.randomVelocity[0]=30;
  legacy.kit(oldKit);legacy.open();
  auto* oldView=IntegrationAccess::view(*legacy.c,0);
  VSTGUI::CPoint first(496.,top+UiCompactLayout::rowKnobY);
  oldView->onMouseDown(first,VSTGUI::CButtonState(VSTGUI::kLButton));
  VSTGUI::CPoint last(496.,top-182.);
  oldView->onMouseMoved(last,VSTGUI::CButtonState(VSTGUI::kLButton));
  oldView->onMouseUp(last,{});
  check(legacy.c->rowRandomVelocity(0)==100,
        "V2 legacy realtime random knob failed");
 }
 click(*v,597.,top+20.);
 click(*v,560.,top+36.+13.);
 check(f.c->rowOutput(0)==8,"V2 direct output selection OUT 9 failed");
 click(*v,590.,top+89.); // choke is beside the row's random-velocity knob
 click(*v,554.,top+36.+4.*27.+12.);
 check(f.c->rowChoke(0)==4,"V2 Choke 4 direct selection failed");
 click(*v,45.,top+UiCompactLayout::baseRow+12.);
 check(IntegrationAccess::selectedRow(*v)==1,
       "V2 instrument header selection failed");
 drag(133.,top+UiCompactLayout::baseRow+UiCompactLayout::rowKnobY,-25.);
 check(f.c->rowGain(0)==100&&f.c->rowGain(1)==117,
       "V2 rotary changed wrong instrument");
 click(*v,133.,top+UiCompactLayout::baseRow+UiCompactLayout::rowKnobY);
 check(IntegrationAccess::activeKnob(*v)==0,"V2 knob capture not active");
 check(f.recall(Fixture::encode(namedKit('Y')))==kResultOk,"V2 recall failed");
 VSTGUI::CPoint stale(133.,0.);
 v->onMouseMoved(stale,VSTGUI::CButtonState(VSTGUI::kLButton));
 v->onMouseUp(stale,{});
 check(f.c->rowGain(0)==130,"V2 stale knob edited restored kit");
 evidence("V2 rotary drag, exact defaults, menus, selection and recall PASS");
}
void testVelocityLaneExistingStepsOnly(){
 Fixture f;
 auto kit=namedKit('X',4);
 kit.grid.fill(0);
 kit.grid[0]=100;kit.grid[2]=90;
 f.kit(kit);f.open();
 auto* v=IntegrationAccess::view(*f.c,0);
 const double top=UiCompactLayout::header;
 const double barY=top+UiCompactLayout::baseRow+4.;
 const auto target=UiCompactLayout::velocityAt(barY,top);
 check(target!=100&&target!=90,"UX fixture must choose a velocity distinct from both source notes");
 const auto col=[](std::size_t step)->double {
   return UiCompactLayout::stepX+static_cast<double>(step)*UiCompactLayout::stepWidth+8.;
 };
 click(*v,1248.,top+88.);
 check(IntegrationAccess::velocityExpanded(*v,0),
       "Velocity blank-gate fixture lane did not expand");
 click(*v,col(1),barY);
 check(f.c->velocity(0,0,1)==0&&
       !f.c->canUndoCurrentPattern(),
       "UX blank velocity bar created an unintended step or Undo");
 {
   VSTGUI::CPoint point(col(0),barY);
   v->onMouseDown(point,VSTGUI::CButtonState(VSTGUI::kLButton));
   point.x=col(1);
   v->onMouseMoved(point,VSTGUI::CButtonState(VSTGUI::kLButton));
   point.x=col(2);
   v->onMouseMoved(point,VSTGUI::CButtonState(VSTGUI::kLButton));
   v->onMouseUp(point,{});
 }
 check(f.c->velocity(0,0,0)==target&&
       f.c->velocity(0,0,2)==target,
       "UX occupied velocity bars did not change during row drag");
 check(f.c->velocity(0,0,1)==0,
       "UX dragging across empty column activated a new note");
 check(f.c->canUndoCurrentPattern()&&f.c->undoCurrentPattern(),
       "UX occupied velocity drag not undoable as one action");
 check(f.c->velocity(0,0,0)==100&&f.c->velocity(0,0,1)==0&&
       f.c->velocity(0,0,2)==90,
       "UX velocity drag Undo did not restore exact original steps");
 // Standard Step buttons remain the explicit way to add notes.
 click(*v,col(1),top+88.);
 check(f.c->velocity(0,0,1)>0,
       "UX standard Step click no longer enables a blank note");
 evidence("UX velocity lane edits occupied notes only; drag/Undo and explicit Step creation PASS");
}
void testRateChangeTransportReset(){
 Fixture f;
 ProcessSetup first{};
 first.sampleRate=48000.;
 first.maxSamplesPerBlock=256;
 first.symbolicSampleSize=kSample32;
 first.processMode=kRealtime;
 check(f.p->setupProcessing(first)==kResultOk,"T28 initial rate setup");
 IntegrationAccess::seedTransportClock(*f.p);
 first.sampleRate=44100.;
 check(f.p->setupProcessing(first)==kResultOk,"T28 rate reconfiguration");
 check(IntegrationAccess::clockReset(*f.p),
       "T28 rate change retained old expected PPQ or phase origin");
 evidence("T28 rate reconfiguration clears expected host PPQ and phase origin PASS");
}
void testRandomVelocityOneShot(){
 Fixture f;
 auto kit=namedKit('X',4);
 kit.grid.fill(0);
 kit.patternLengths[0]=16;
 kit.grid[0]=127;kit.grid[4]=90;kit.grid[12]=1;
 kit.grid[20]=64; // inactive second bar: must NOT change in 1T
 kit.grid[kSteps]=88; // different instrument: untouched
 kit.grid[kRows*kSteps]=100; // different pattern: untouched
 f.kit(kit);f.open();
 check(!f.c->randomizeRowVelocity(2),"RANDOM empty row should be inert");
 check(!f.c->canUndoCurrentPattern(),"RANDOM empty row incorrectly added undo");
 check(!f.c->randomizeRowVelocity(4),"RANDOM row outside visible kit allowed");
 check(f.c->randomizeRowVelocity(0),"RANDOM occupied row was rejected");
 for(const std::size_t step:{0u,4u,12u}){
   const auto before=kit.grid[step];
   const auto after=f.c->velocity(0,0,step);
   check(after>0&&after<=127&&after!=before,
         "RANDOM occupied velocity did not change visibly");
 }
 for(std::size_t step=0;step<kSteps;++step) {
   if(step!=0&&step!=4&&step!=12)
     check(f.c->velocity(0,0,step)==kit.grid[step],
           "RANDOM changed empty or hidden step");
 }
 check(f.c->velocity(0,1,0)==88&&f.c->velocity(1,0,0)==100,
       "RANDOM changed a different instrument or pattern");
 check(f.c->canUndoCurrentPattern(),"RANDOM missing single-pattern Undo");
 check(f.c->undoCurrentPattern(),"RANDOM cannot Undo action");
 for(std::size_t step=0;step<kSteps;++step)
   check(f.c->velocity(0,0,step)==kit.grid[step],
         "RANDOM undo did not restore complete source row");
 check(!f.c->canUndoCurrentPattern(),"RANDOM should consume exactly one undo");
 check(f.c->togglePatternLength(0),"RANDOM setup 2T");
 check(f.c->randomizeRowVelocity(0)&&f.c->velocity(0,0,20)!=64,
       "RANDOM 2T failed to edit second bar");
 check(f.c->undoCurrentPattern()&&f.c->velocity(0,0,20)==64,
       "RANDOM second-bar undo failed");
 check(f.c->togglePatternLength(0),"Button test return to one bar");
 auto* view=IntegrationAccess::view(*f.c,0);
 const auto before=f.c->velocity(0,0,0);
 click(*view,496.,UiCompactLayout::header+89.);
 check(f.c->velocity(0,0,0)!=before,"Button click did not randomize velocity");
 check(f.c->velocity(0,0,1)==0&&f.c->velocity(0,0,20)==64,"Button changed empty/inactive step");
 check(f.c->undoCurrentPattern()&&f.c->velocity(0,0,0)==before,"Button Undo failed");
 evidence("RANDOM one-shot occupied-only, 1T/2T, isolated row/pattern and Undo PASS");
}
void testFillActivePatternLength(){
 // V17: all nine FILL variants, including CLEAR, modify only the visible
 // musical bar in 1T while preserving stored bar-two notes for later 2T.
 Fixture f;
 KitState kit=namedKit('X',4);
 kit.grid.fill(0);
 kit.grid[kSteps+0]=90;kit.grid[kSteps+4]=100;
 kit.grid[kSteps+19]=37;kit.grid[kSteps+30]=65;
 kit.grid[0]=75; // other row, must be untouched
 kit.grid[kRows*kSteps+kSteps+2]=61; // other pattern, untouched
 const std::array<RowFill,9> modes{
   RowFill::Every2,RowFill::Every4,RowFill::Every8,
   RowFill::Snare24,RowFill::All,RowFill::Clear,RowFill::Offbeat8,
   RowFill::Gallop,RowFill::SnareGhost
 };
 auto state=[&]{
   ResizableMemoryIBStream stream;KitState saved;
   check(f.p->getState(&stream)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                          static_cast<std::size_t>(stream.getCursor()),saved),
         "V17 actual processor project snapshot");
   return saved;
 };
 for(const auto active:{16u,32u}) {
   kit.patternLengths[0]=static_cast<std::uint8_t>(active);
   for(const auto mode:modes) {
     f.kit(kit);
     check(f.c->patternLength(0)==active,"V17 incorrect 1T/2T restore");
     check(f.c->fillRow(1,mode),"V17 actual controller FILL rejected");
     const auto saved=state();
     const auto expected=filledRow(mode);
     for(std::size_t index=0;index<kit.grid.size();++index){
       const auto row=index%(kRows*kSteps)/kSteps;
       const auto step=index%kSteps;
       const auto wanted=index<kRows*kSteps&&row==1&&step<active?
            expected[step]:kit.grid[index];
       check(saved.grid[index]==wanted,
             "V17 FILL/CLEAR modified hidden bar, other row or pattern");
     }
     check(f.c->canUndoCurrentPattern()&&f.c->undoCurrentPattern()&&
           state().grid==kit.grid,
           "V17 FILL/CLEAR Undo failed to restore complete original pattern");
   }
 }
 // Switch from 1T to 2T without clearing the stored second-bar material.
 kit.patternLengths[0]=16;
 f.kit(kit);
 check(f.c->fillRow(1,RowFill::Clear),"V17 1T CLEAR rejected");
 check(f.c->togglePatternLength(0)&&f.c->patternLength(0)==32,
       "V17 cannot restore 2T after FILL/CLEAR");
 check(f.c->velocity(0,1,19)==37&&f.c->velocity(0,1,30)==65,
       "V17 1T FILL/CLEAR destroyed previously hidden bar two");
 evidence("V17 nine row FILL/CLEAR variants preserve inactive steps, Undo and 2T return PASS");
}
void testVelocityInactiveBarProtection(){
 // V16: 1T editing operations must never change stored steps 17-32.
 // These steps must reappear at their original velocities on returning to 2T.
 Fixture f;auto kit=namedKit('X',4);
 kit.grid.fill(0);kit.patternLengths[0]=16;
 kit.grid[0]=90;kit.grid[4]=110;kit.grid[20]=64;
 kit.grid[kSteps+1]=60;kit.grid[kSteps+21]=33;
 kit.grid[kRows*kSteps]=77; // other pattern must remain unchanged
 f.kit(kit);f.open();
 auto gridAt=[&]{
   ResizableMemoryIBStream b;KitState state;
   check(f.p->getState(&b)==kResultOk&&
      KitCodec::decode(static_cast<const std::uint8_t*>(b.getData()),
                       static_cast<std::size_t>(b.getCursor()),state),
      "V16 actual processor state");
   return state;
 };
 check(f.c->patternLength(0)==16,"V16 1T fixture lost pattern length");
 check(f.c->varyPatternVelocity(),"V16 1T velocity vary rejected");
 const auto varied=gridAt();
 for(std::size_t row=0;row<kRows;++row)
   for(std::size_t step=16;step<kSteps;++step)
     check(varied.grid[row*kSteps+step]==kit.grid[row*kSteps+step],
           "V16 VELO VARIIEREN silently changed hidden bar two");
 check(varied.grid[kRows*kSteps]==77&&
       varied.grid[1]==0&&varied.grid[kSteps]==0,
       "V16 vary modified another pattern or introduced a note");
 check(f.c->undoCurrentPattern(),"V16 vary not undoable");
 check(gridAt().grid==kit.grid,"V16 vary Undo failed to restore full pattern");
 check(f.c->resetPatternVelocity(),"V16 1T velocity reset rejected");
 const auto reset=gridAt();
 check(reset.grid[0]==100&&reset.grid[4]==100&&
       reset.grid[kSteps+1]==100&&
       reset.grid[20]==64&&reset.grid[kSteps+21]==33&&
       reset.grid[kRows*kSteps]==77,
       "V16 1T reset touched bar 2/other pattern or missed active notes");
 check(f.c->undoCurrentPattern()&&gridAt().grid==kit.grid,
       "V16 1T reset Undo failed");
 check(f.c->togglePatternLength(0)&&f.c->patternLength(0)==32,
       "V16 could not return to two bars");
 check(f.c->velocity(0,0,20)==64&&f.c->velocity(0,1,21)==33,
       "V16 hidden notes changed when restored to 2T");
 check(f.c->resetPatternVelocity(),"V16 2T reset rejected");
 const auto reset2=gridAt();
 check(reset2.grid[20]==100&&reset2.grid[kSteps+21]==100,
       "V16 2T reset failed to reach second bar");
 check(f.c->undoCurrentPattern()&&gridAt().grid[20]==64&&
       gridAt().grid[kSteps+21]==33,
       "V16 2T undo did not recover hidden velocities");
 evidence("V16 active 1T VELO VARIIEREN/100 preserves second bar; 2T and Undo PASS");
}
void testNewStepDefault100(){
 Fixture f;
 auto kit=namedKit('X',4);
 kit.grid.fill(0);
 f.kit(kit);f.open();
 check(f.c->velocity(0,0,2)==0,"default step fixture not empty");
 check(f.c->toggleStep(0,0,2),"new step toggle rejected");
 check(f.c->velocity(0,0,2)==100,"new step did not receive Velocity 100");
 check(f.c->toggleStep(0,0,2)&&f.c->velocity(0,0,2)==0,
       "second step click must disable note");
 check(f.c->setStepVelocity(0,0,2,37),"setup explicit Velocity 37 failed");
 check(f.c->toggleStep(0,0,2)&&f.c->velocity(0,0,2)==0,
       "click on existing non-default Velocity must disable note");
 check(f.c->toggleStep(0,0,2)&&f.c->velocity(0,0,2)==100,
       "recreated step must use default Velocity 100");
 check(f.c->velocity(0,1,2)==0&&f.c->velocity(1,0,2)==0,
       "toggle changed another row or pattern");
 evidence("SOR01 real Controller toggles 0 -> 100 -> 0; previous non-default 37 -> 0 -> 100 PASS");
}
void testVelocityResetPattern(){
 Fixture f;
 auto kit=namedKit('X',4);
 kit.grid.fill(0);
 kit.grid[0]=1;kit.grid[4]=127;kit.grid[20]=64;
 kit.grid[kSteps]=88; // second instrument, same pattern
 kit.grid[kRows*kSteps]=72; // next pattern must remain untouched
 f.kit(kit);f.open();
 check(f.c->resetPatternVelocity(),"VELO 100 reset rejected");
 for(const std::size_t index : {std::size_t{0},std::size_t{4},std::size_t{20},std::size_t{kSteps}})
   check(f.c->velocity(0,index/kSteps,index%kSteps)==100,
         "VELO 100 did not reset occupied step to 100");
 check(f.c->velocity(0,0,1)==0&&f.c->velocity(0,1,1)==0,
       "VELO 100 created notes on empty steps");
 check(f.c->velocity(1,0,0)==72,"VELO 100 modified another pattern");
 check(f.c->canUndoCurrentPattern()&&f.c->undoCurrentPattern(),
       "VELO 100 must be one undoable operation");
 check(f.c->velocity(0,0,0)==1&&f.c->velocity(0,0,4)==127&&
       f.c->velocity(0,0,20)==64&&f.c->velocity(0,1,0)==88,
       "VELO 100 Undo did not restore exact prior velocities");
 check(!f.c->canUndoCurrentPattern(),"VELO 100 left extra undo operations");
 // Exercise actual native popup hit zones, not just the Controller API.
 click(*IntegrationAccess::view(*f.c,0),900.,114.); // PATTERN menu
 click(*IntegrationAccess::view(*f.c,0),900.,281.); // VELO 100 (item six)
 check(f.c->velocity(0,0,0)==100&&f.c->velocity(0,0,4)==100&&
       f.c->velocity(0,1,0)==100&&f.c->velocity(0,0,1)==0,
       "VELO 100 popup did not reset occupied notes only");
 click(*IntegrationAccess::view(*f.c,0),900.,114.);
 click(*IntegrationAccess::view(*f.c,0),900.,229.); // Undo (item four)
 check(f.c->velocity(0,0,0)==1&&f.c->velocity(0,0,4)==127&&
       f.c->velocity(0,1,0)==88&&f.c->velocity(0,0,1)==0,
       "VELO 100 popup Undo did not restore previous velocities");
 evidence("VELO 100 all occupied notes, no new notes, isolated pattern and single Undo PASS");
}
void testPatternDurationClipboard(){
 // P19: COPY/PASTE and DUPLIZIEREN must transfer both stored steps
 // and their musical length, and Undo must restore both in one operation.
 Fixture f;
 KitState kit=namedKit('X',4);
 kit.grid.fill(0);
 kit.grid[0]=91;kit.grid[20]=64;
 kit.grid[kRows*kSteps]=42;kit.grid[kRows*kSteps+20]=37;
 kit.patternLengths[0]=16;kit.patternLengths[1]=32;kit.patternLengths[2]=32;
 f.kit(kit);
 auto state=[&] {
   ResizableMemoryIBStream stream;KitState value;
   check(f.p->getState(&stream)==kResultOk&&
      KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                       static_cast<std::size_t>(stream.getCursor()),value),
      "P19 processor state decode");
   return value;
 };
 check(f.c->copyCurrentPattern(),"P19 copy one-bar A");
 check(f.c->setParamNormalized(kPatternParam,1./7.)==kResultOk,
       "P19 select B");
 check(f.c->pasteCurrentPattern(),"P19 paste one bar into 2T");
 auto saved=state();
 check(saved.patternLengths[1]==16&&f.c->patternLength(1)==16&&
       saved.grid[kRows*kSteps]==91&&saved.grid[kRows*kSteps+20]==64,
       "P19 paste failed to atomically transfer length and hidden steps");
 check(f.c->undoCurrentPattern(),"P19 paste Undo");
 saved=state();
 check(saved.patternLengths[1]==32&&f.c->patternLength(1)==32&&
       saved.grid[kRows*kSteps]==42&&saved.grid[kRows*kSteps+20]==37,
       "P19 paste Undo failed to restore B's original 2T grid and duration");
 check(f.c->pasteCurrentPattern()&&f.c->duplicateCurrentPattern(),
       "P19 paste and duplicate to empty C");
 saved=state();
 check(saved.patternLengths[2]==16&&
       saved.grid[2*kRows*kSteps]==91&&saved.grid[2*kRows*kSteps+20]==64,
       "P19 duplicate failed to copy 1T musical length");
 check(f.c->setParamNormalized(kPatternParam,2./7.)==kResultOk&&
       f.c->undoCurrentPattern(),"P19 duplicate Undo");
 saved=state();
 check(saved.patternLengths[2]==32&&
       saved.grid[2*kRows*kSteps]==0&&saved.grid[2*kRows*kSteps+20]==0,
       "P19 duplicate Undo failed to restore previous empty 2T pattern");
 // Check other direction: 2T copied over an existing 1T pattern.
 KitState reverse=kit;reverse.patternLengths[0]=32;reverse.patternLengths[1]=16;
 f.kit(reverse);
 // Selecting A explicitly is required: the preceding host-selected C can
 // legitimately survive Component recall (RE07). The clipboard source
 // must not depend on the host's prior active pattern.
 check(f.c->setParamNormalized(kPatternParam,0.)==kResultOk&&
       f.c->currentPattern()==0,"P19 explicitly select source A after kit recall");
 check(f.c->copyCurrentPattern()&&
       f.c->setParamNormalized(kPatternParam,1./7.)==kResultOk&&
       f.c->pasteCurrentPattern(),"P19 2T->1T paste");
 saved=state();
 check(saved.patternLengths[1]==32&&f.c->patternLength(1)==32&&
       saved.grid[kRows*kSteps+20]==64,
       "P19 paste 2T did not activate second bar");
 check(f.c->undoCurrentPattern(),"P19 2T->1T Undo");
 saved=state();
 check(saved.patternLengths[1]==16&&saved.grid[kRows*kSteps+20]==37,
       "P19 2T->1T undo duration or step failure");
 // Identical note contents still require the length change and Undo.
 KitState matching=kit;
 matching.grid[kRows*kSteps]=matching.grid[0];
 matching.grid[kRows*kSteps+20]=matching.grid[20];
 f.kit(matching);
 check(f.c->setParamNormalized(kPatternParam,0.)==kResultOk&&
       f.c->currentPattern()==0,"P19 explicitly select matching source A");
 check(f.c->copyCurrentPattern()&&
       f.c->setParamNormalized(kPatternParam,1./7.)==kResultOk&&
       f.c->pasteCurrentPattern(),"P19 length-only paste rejected");
 check(state().patternLengths[1]==16,"P19 length-only edit not persisted");
 check(f.c->undoCurrentPattern()&&state().patternLengths[1]==32,
       "P19 length-only undo failed");
 // A malformed combined control message must be rejected *before* changing
 // either pattern bytes or the musical duration.
 const auto unaffected=state();
 PatternSteps malformedSteps{};malformedSteps[0]=125;
 HostMessage malformed;malformed.setMessageID("Drumator.PatternReplace");
 auto* bad=malformed.getAttributes();
 check(bad&&bad->setInt("Pattern",1)==kResultOk&&
       bad->setBinary("Steps",malformedSteps.data(),
                     static_cast<uint32>(malformedSteps.size()))==kResultOk&&
       bad->setInt("Length",24)==kResultOk,
       "P19 invalid-length message setup");
 check(f.p->notify(&malformed)==kInvalidArgument,
       "P19 malformed length not rejected");
 check(state().grid==unaffected.grid&&
       state().patternLengths==unaffected.patternLengths,
       "P19 malformed length partially committed pattern");
 // Legacy hosts/controllers have always sent PatternReplace with Steps only:
 // they must continue working without resetting any pattern duration.
 KitState oldProtocol=matching;
 oldProtocol.patternLengths[7]=16;
 f.kit(oldProtocol);
 HostMessage legacy;legacy.setMessageID("Drumator.PatternReplace");
 auto* attrs=legacy.getAttributes();
 check(attrs&&attrs->setInt("Pattern",7)==kResultOk&&
       attrs->setBinary("Steps",malformedSteps.data(),
                        static_cast<uint32>(malformedSteps.size()))==kResultOk,
       "P19 legacy message setup");
 check(f.p->notify(&legacy)==kResultOk&&
       state().patternLengths[7]==16&&
       state().grid[7*kRows*kSteps]==125,
       "P19 legacy Steps-only message broke duration or edit compatibility");
 evidence("P19 COPY/PASTE/DUPLICATE length 1T/2T, Undo, legacy and invalid-length PASS");
}
void testPatternDuplicateMenu(){
 Fixture f;
 KitState kit=namedKit('X',4);
 kit.grid.fill(0);
 kit.grid[0]=90;kit.grid[4]=60;kit.grid[20]=25;
 kit.patternLengths[0]=32;
 f.kit(kit);f.open();
 auto* view=IntegrationAccess::view(*f.c,0);
 check(f.c->canDuplicateCurrentPattern(),"P18 initial pattern not duplicable");
 auto getGrid=[&]{
   ResizableMemoryIBStream stream;KitState data;
   check(f.p->getState(&stream)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                          static_cast<std::size_t>(stream.getCursor()),data),
         "P18 decode actual processor project");
   return data.grid;
 };
 const auto before=getGrid();
 click(*view,900.,114.); // PATTERN popup
 click(*view,900.,307.); // seventh and final action: DUPLIZIEREN
 const auto after=getGrid();
 for(std::size_t i=0;i<kRows*kSteps;++i){
   check(after[i]==before[i],"P18 duplicate changed source pattern");
   check(after[kRows*kSteps+i]==before[i],
         "P18 native DUPLIZIEREN menu did not copy entire source to pattern B");
 }
 check(after[2*kRows*kSteps]==0,"P18 duplicate wrote a third pattern");
 check(f.c->velocity(1,0,20)==25,"P18 controller not updated for copied pattern");
 // Existing action returns success if the host refuses selecting the copy:
 // source remains valid, and manually selecting B still permits Undo.
 check(f.c->setParamNormalized(kPatternParam,1./7.)==kResultOk,
       "P18 cannot navigate to duplicated pattern B");
 check(f.c->canUndoCurrentPattern()&&f.c->undoCurrentPattern(),
       "P18 duplicate did not create undo history for target");
 check(getGrid()==before,"P18 target undo did not restore completely empty B");
 // With all eight patterns occupied, the menu action is disabled and
 // neither host nor controller may overwrite an existing pattern.
 KitState full=kit;
 for(std::size_t p=0;p<kPatterns;++p)
   full.grid[p*kRows*kSteps]=static_cast<std::uint8_t>(p+1);
 f.kit(full);
 check(!f.c->canDuplicateCurrentPattern(),"P18 full bank incorrectly duplicable");
 const auto fullBefore=getGrid();
 click(*view,900.,114.);
 click(*view,900.,307.);
 check(getGrid()==fullBefore,"P18 full-bank DUPLIZIEREN overwrote a pattern");
 // An empty source also must never be duplicated into another slot.
 KitState empty=kit;empty.grid.fill(0);
 f.kit(empty);
 check(!f.c->canDuplicateCurrentPattern(),"P18 empty pattern offered duplicate");
 click(*view,900.,114.);
 click(*view,900.,307.);
 check(getGrid()==empty.grid,"P18 empty source duplicated unexpectedly");
 evidence("P18 native DUPLIZIEREN menu, whole-grid copy, Undo, full bank and empty source PASS");
}
void testPatternLengthControls(){
 Fixture f;
 auto kit=namedKit('X');
 // Row five must actually be empty for the existing safe-removal contract.
 kit.displayNames[4].clear();
 kit.names[4].clear();
 f.kit(kit);f.open();
 auto* v=IntegrationAccess::view(*f.c,0);
 check(f.c->patternLength(0)==32,"UX pattern length initial state");
 click(*v,322.,UiCompactLayout::headerButtonTop+13.);
 check(f.c->patternLength(0)==16,"UX visible 1 TAKT control failed");
 const double blockedX=UiCompactLayout::stepX+20.*UiCompactLayout::stepWidth+8.;
 const auto oldHidden=f.c->velocity(0,0,20);
 click(*v,blockedX,UiCompactLayout::header+88.);
 check(f.c->velocity(0,0,20)==oldHidden,"UX clicked inactive step 21 in 1T");
 click(*v,1248.,UiCompactLayout::header+88.);
 click(*v,blockedX,UiCompactLayout::header+UiCompactLayout::baseRow+12.);
 check(f.c->velocity(0,0,20)==oldHidden,"UX edited inactive velocity in 1T");
 click(*v,322.,UiCompactLayout::headerButtonTop+13.);
 check(f.c->patternLength(0)==16,"UX selected length not idempotent");
 click(*v,414.,UiCompactLayout::headerButtonTop+13.);
 check(f.c->patternLength(0)==32,"UX visible 2 TAKTE control failed");
 click(*v,900.,114.);
 click(*v,900.,150.);
 check(f.c->patternLength(0)==32,"UX copy changed length via duplicate menu");
 const double bottom=UiCompactLayout::header+UiCompactLayout::baseRow*4.+UiCompactLayout::velocityRow;
 click(*v,76.,bottom+17.);
 check(f.c->visibleRows()==5,"UX + ZEILE not clickable");
 click(*v,160.,bottom+17.);
 check(f.c->visibleRows()==4,"UX - ZEILE not clickable");
 evidence("UX direct length and no duplicate pattern menu control PASS");
}
void testControlExtremeDeltas(){
 Fixture f;f.kit(namedKit('D',4));
 // Pitch updates rebuild a prepared sample; the processor needs the host
 // sample rate even for a kit whose audio sample is not yet loaded.
 ProcessSetup setup{};
 setup.sampleRate=48000.;setup.maxSamplesPerBlock=256;
 setup.symbolicSampleSize=kSample32;setup.processMode=kRealtime;
 check(f.p->setupProcessing(setup)==kResultOk,"D11 pitch rate not initialized");
 constexpr int upper=std::numeric_limits<int>::max();
 constexpr int lower=std::numeric_limits<int>::min();
 check(f.c->adjustMasterVolume(upper)&&f.c->masterVolume()==200,"D11 master upper");
 check(f.c->adjustMasterVolume(lower)&&f.c->masterVolume()==0,"D11 master lower");
 check(f.c->adjustHumanize(upper)&&f.c->humanizePercent()==100,"D11 humanize upper");
 check(f.c->adjustHumanize(lower)&&f.c->humanizePercent()==0,"D11 humanize lower");
 check(f.c->adjustSwing(upper)&&f.c->swingPercent()==50,"D11 swing upper");
 check(f.c->adjustSwing(lower)&&f.c->swingPercent()==0,"D11 swing lower");
 check(f.c->adjustRandomVelocity(0,upper)&&f.c->rowRandomVelocity(0)==100,"D11 random upper");
 check(f.c->adjustRandomVelocity(0,lower)&&f.c->rowRandomVelocity(0)==0,"D11 random lower");
 check(f.c->adjustRowPitch(0,upper)&&f.c->rowPitch(0)==24,"D11 pitch upper");
 check(f.c->adjustRowPitch(0,lower)&&f.c->rowPitch(0)==-24,"D11 pitch lower");
 check(f.c->adjustRowMix(0,false,upper)&&f.c->rowGain(0)==200,"D11 volume upper");
 check(f.c->adjustRowMix(0,false,lower)&&f.c->rowGain(0)==0,"D11 volume lower");
 check(f.c->adjustRowMix(0,true,upper)&&f.c->rowPan(0)==100,"D11 pan upper");
 check(f.c->adjustRowMix(0,true,lower)&&f.c->rowPan(0)==-100,"D11 pan lower");
 ResizableMemoryIBStream stream;KitState saved;
 check(f.p->getState(&stream)==kResultOk&&
       KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                       static_cast<std::size_t>(stream.getCursor()),saved),
       "D11 saved processor decode");
 check(saved.masterVolume==0&&saved.humanize==0&&saved.swing==0,
       "D11 saved global controls mismatch");
 evidence("D11 extreme control deltas, controller endpoints and processor state PASS");
}
void testSongCountExtremeDelta(){
 Fixture f;auto kit=namedKit('C',4);
 check(kit.chain.setCount(3)&&kit.chain.configure(0,5,16),
       "S10 song count fixture");
 f.kit(kit);
 auto checkCount=[&](std::size_t expected){
   check(f.c->songCount()==expected,"S10 controller count mismatch");
   ResizableMemoryIBStream stream;KitState saved;
   check(f.p->getState(&stream)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                     static_cast<std::size_t>(stream.getCursor()),saved),
         "S10 processor state decode");
   check(saved.chain.count()==expected&&saved.chain.entry(0).pattern==5&&
         saved.chain.entry(0).steps==16,
         "S10 processor count or existing song entry mismatch");
 };
 check(f.c->adjustSongCount(std::numeric_limits<int>::max()),
       "S10 INT_MAX delta rejected");
 checkCount(SongChain::kCapacity);
 check(f.c->adjustSongCount(std::numeric_limits<int>::min()),
       "S10 INT_MIN delta rejected");
 checkCount(1);
 check(f.c->adjustSongCount(2),"S10 ordinary +2 rejected");
 checkCount(3);
 check(f.c->adjustSongCount(-1),"S10 ordinary -1 rejected");
 checkCount(2);
 evidence("S10 song count INT_MIN/MAX saturation, normal edits and persisted state PASS");
}
void testSongEntryExtremeDelta(){
 Fixture f;auto kit=namedKit('X',4);
 kit.songEnabled=1;
 check(kit.chain.setCount(3)&&kit.chain.configure(0,7,32)&&
       kit.chain.configure(1,0,8)&&kit.chain.configure(2,3,16),
       "S09 song test fixture");
 f.kit(kit);
 auto checkState=[&](std::uint8_t p,std::uint8_t len) {
   const auto e=f.c->songEntry(0);
   check(e.pattern==p&&e.steps==len,"S09 controller wrong song entry");
   ResizableMemoryIBStream stream;KitState saved;
   check(f.p->getState(&stream)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                 static_cast<std::size_t>(stream.getCursor()),saved),
         "S09 processor state decode");
   const auto persisted=saved.chain.entry(0);
   check(persisted.pattern==p&&persisted.steps==len,
         "S09 processor/controller Song entry mismatch");
 };
 check(f.c->editSongEntry(0,1,1),"S09 ordinary +1 rejected");
 checkState(0,8);
 check(f.c->editSongEntry(0,-1,-1),"S09 ordinary -1 rejected");
 checkState(7,32);
 // All these arguments were previously liable to signed overflow or
 // negative indexing when the native API was invoked directly.
 const auto big=std::numeric_limits<int>::max();
 const auto minimumDelta=std::numeric_limits<int>::min();
 check(f.c->editSongEntry(0,big,big),"S09 INT_MAX delta rejected");
 checkState(6,8); // 7+(INT_MAX mod 8)=6; length 32 -> 8
 check(f.c->editSongEntry(0,minimumDelta,minimumDelta),"S09 INT_MIN delta rejected");
 checkState(6,16); // INT_MIN mod 8=0, mod 3=-2: 8 -> 16
 check(f.c->editSongEntry(0,0,0),"S09 zero delta rejected");
 checkState(6,16);
 check(!f.c->editSongEntry(99,1,1),"S09 invalid song slot accepted");
 checkState(6,16);
 evidence("S09 song editor signed modular delta INT_MIN/MAX, normal edits and recall PASS");
}
void testPagedSongEditor(){
 Fixture f;
 auto kit=namedKit('X',4);
 check(kit.chain.setCount(16),"UX song fixture capacity");
 f.kit(kit);f.open();
 auto* v=IntegrationAccess::view(*f.c,0);
 click(*v,1077.,114.); // SONG EDIT opens
 check(IntegrationAccess::songPage(*v)==0,"UX song page initial selection");
 // The page before slot 17 must be reachable even when only 16 are configured.
 click(*v,1170.,181.);
 check(IntegrationAccess::songPage(*v)==1,"UX song next page unavailable at boundary");
 // This page has no existing position: its Pattern/Length controls must
 // neither highlight nor edit a position that is hidden on page one.
 const auto originalHidden=f.c->songEntry(0);
 click(*v,115.+2.*56.+25.,352.);
 click(*v,707.,352.);
 check(f.c->songEntry(0).pattern==originalHidden.pattern&&
       f.c->songEntry(0).steps==originalHidden.steps,
       "UX empty Song page edited offscreen entry");

 click(*v,17.,222.); // first empty slot 17 is an add action
 check(f.c->songCount()==17&&IntegrationAccess::songSlot(*v)==16,
       "UX song add slot 17 via its own cell failed");
 captureVstgui(*v,"ux-song-page-2",1.0);
 click(*v,115.+2.*56.+25.,352.); // select Pattern C for entry 17
 check(f.c->songEntry(16).pattern==2,"UX song pattern selector missed selected entry");
 click(*v,707.,352.); // half bar
 check(f.c->songEntry(16).steps==8,"UX song length selector missed selected entry");
 const auto snapshot=IntegrationAccess::snapshot(*f.c);
 click(*v,167.,222.); // 4px gutter between song items, must be inert
 check(IntegrationAccess::songSlot(*v)==16&&
       IntegrationAccess::snapshot(*f.c)==snapshot,"UX song gutter changed entry");
 click(*v,350.,222.); // slot 19 is not yet available: inactive
 check(f.c->songCount()==17,"UX future Song item should not implicitly add");
 click(*v,1032.,181.);
 check(IntegrationAccess::songPage(*v)==0,"UX song previous page failed");
 check(IntegrationAccess::songSlot(*v)==0,
       "UX previous page must select first visible existing entry");

 click(*v,1170.,222.); // slot 8 on page 1
 check(IntegrationAccess::songSlot(*v)==7,"UX song cell selection hit unexpected slot");
 click(*v,1060.,352.); // add 18th slot (automatically selects its page)
 check(f.c->songCount()==18&&IntegrationAccess::songPage(*v)==1&&
       IntegrationAccess::songSlot(*v)==17,"UX Song + EINTRAG did not auto-page");
 click(*v,1172.,352.); // delete last (must select 17)
 check(f.c->songCount()==17&&IntegrationAccess::songSlot(*v)==16,
       "UX Song last-entry removal selection incorrect");
 check(f.c->songEntry(16).pattern==2&&f.c->songEntry(16).steps==8,
       "UX Song pagination corrupted edited entry");
 auto recalled=namedKit('Y',4);
 check(f.recall(Fixture::encode(recalled))==kResultOk,
       "UX Song recall reduced chain rejected");
 check(IntegrationAccess::songPage(*v)==0&&
       IntegrationAccess::songSlot(*v)<f.c->songCount(),
       "UX Song page/selection left outside recalled chain");
 evidence("UX paged Song editor selection, no offscreen edits, recall and 64-slot navigation PASS");
}
void testSongCompactViewport(){
 Fixture f;f.kit(namedKit('X',4));f.open();
 auto* v=IntegrationAccess::view(*f.c,0);
 check(v->getViewSize().getHeight()==734.,"UX expected four spacious rows");
 click(*v,1077.,114.); // Song edit (compact)
 check(v->getViewSize().getHeight()==432.,"UX Song editor failed to become compact");
 captureVstgui(*v,"ux-song-compact-432",1.0);
 click(*v,1077.,114.); // return to Step grid
 check(v->getViewSize().getHeight()==734.,"UX Step editor height not restored");
 // Reject the enlargement after Song edit. Users must still be able to
 // reach every instrument, not skip rows in a truncated viewport.
 click(*v,1077.,114.);
 check(v->getViewSize().getHeight()==432.,"UX Song second open");
 f.frames[0].accept=false;
 click(*v,1077.,114.); // host refuses resize back to 682
 check(v->getViewSize().getHeight()==432.&&v->visibleRowsForHit()==1,
       "UX refused host resize did not degrade to one reachable row");
 const auto bottom=UiCompactLayout::header+UiCompactLayout::baseRow;
 click(*v,1190.,bottom+17.); // next page: row 2 within refused 370px client
 check(IntegrationAccess::firstRow(*v)==1&&v->visibleRowsForHit()==1,
       "UX compact fallback skipped rows when paging");
 click(*v,45.,UiCompactLayout::header+17.);
 check(IntegrationAccess::selectedRow(*v)==1,"UX compact fallback row 2 not selectable");
 f.frames[0].accept=true;
 click(*v,1090.,bottom+17.); // back to rows 1..4, now resize accepted
 check(IntegrationAccess::firstRow(*v)==0&&
       v->getViewSize().getHeight()==734.&&v->visibleRowsForHit()==4,
       "UX compact fallback cannot recover after host accepts again");
 evidence("UX 370px Song viewport, return to Steps and refused-resize row navigation PASS");
}
void testControllerMetadataCommit(){
 Fixture f;auto initial=namedKit('X');
 initial.displayNames[1].clear(); // row 2 has no custom label
 f.kit(initial);
 // The controller must not notify a successful processor mutation before
 // allocating its replacement display label (OOM before commit).
 std::string replacement(64,'R');
 const auto before=IntegrationAccess::snapshot(*f.c);
 allocationFail=0;
 bool escaped=false,ok=false;
 try {ok=f.c->renameRow(0,replacement);}
 catch(const std::bad_alloc&) {escaped=true;}
 allocationFail=-1;
 check(!escaped&&!ok&&IntegrationAccess::snapshot(*f.c)==before,
       "N11 Controller rename OOM escaped or mutated local state");
 auto savedKit=[&] {
   ResizableMemoryIBStream stream;KitState state;
   check(f.p->getState(&stream)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                          static_cast<std::size_t>(stream.getCursor()),state),
         "N11 processor state decode");
   return state;
 };
 check(savedKit().displayNames[0]==initial.displayNames[0],
       "N11 Controller rename OOM already changed processor");
 check(f.c->renameRow(0,replacement)&&
       f.c->customName(0)==replacement&&
       savedKit().displayNames[0]==replacement,
       "N11 controller/processor success inconsistent");
 // The actual WAV load must record the filename/path only after a successful
 // processor load. Clear must leave a valid, synchronized empty slot.
 prepareAudio(*f.p);
 const auto candidate=constantStereo(.25f,-.25f);
 const auto path=std::filesystem::current_path()/
       "125A-Drumator-N11-test-sample.wav";
 { std::ofstream out(path,std::ios::binary|std::ios::trunc);
   check(out.good(),"N11 create sample file");
   out.write(reinterpret_cast<const char*>(candidate.data()),
             static_cast<std::streamsize>(candidate.size()));
   check(out.good(),"N11 sample write failed");
 }
 const auto pathText=path.string();
 check(f.c->loadWav(1,pathText)==Controller::LoadStatus::Accepted,
       "N11 processor rejected valid sample file");
 check(IntegrationAccess::paths(*f.c)[1]==pathText&&
       savedKit().names[1]==path.filename().string()&&
       f.c->label(1)==path.filename().string(),
       "N11 loaded sample filename/path/processor state diverged");
 check(f.c->clearSample(1)&&IntegrationAccess::paths(*f.c)[1].empty()&&
       f.c->label(1)=="EMPTY - LOAD WAV"&&
       savedKit().names[1].empty()&&savedKit().wav[1].empty(),
       "N11 clear sample did not atomically reset metadata");
 std::error_code ec;std::filesystem::remove(path,ec);
 check(!ec,"N11 fixture sample cleanup failed");
 f.p->setProcessing(false);f.p->setActive(false);
 evidence("N11 Controller metadata allocation, WAV load and clear consistency PASS");
}
void testRealSampleFolderStepping(){
 // N12: exercise genuine controller < > sample browsing, not only the
 // pure adjacentSample filename helper. Invalid neighbors are skipped
 // without changing the loaded sound, kit bytes or visible filename.
 Fixture f;
 auto fixture=namedKit('X');fixture.displayNames[0].clear();
 f.kit(fixture);
 prepareAudio(*f.p);
 const auto folder=std::filesystem::current_path()/"125A-Drumator-N12-SampleBrowse";
 std::filesystem::create_directories(folder);
 const auto A=folder/"A-good.wav";
 const auto badB=folder/"B-broken.wav";
 const auto C=folder/"C-good.wav";
 const auto badD=folder/"D-broken.wav";
 const auto wavA=constantStereo(.25f,-.25f);
 const auto wavC=constantStereo(-.5f,.125f);
 auto writeFile=[&](const std::filesystem::path& p,const std::vector<std::uint8_t>& bytes){
   std::ofstream out(p,std::ios::binary|std::ios::trunc);
   check(out.good(),"N12 cannot create folder fixture");
   out.write(reinterpret_cast<const char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
   check(out.good(),"N12 cannot write folder fixture");
 };
 writeFile(A,wavA);
 writeFile(badB,std::vector<std::uint8_t>(64,0));
 writeFile(C,wavC);
 writeFile(badD,std::vector<std::uint8_t>(64,0));
 auto snapshot=[&]{
   ResizableMemoryIBStream stream;KitState state;
   check(f.p->getState(&stream)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                          static_cast<std::size_t>(stream.getCursor()),state),
         "N12 processor project snapshot");
   return state;
 };
 const auto aPath=A.string(),cPath=C.string();
 check(f.c->loadWav(0,aPath)==Controller::LoadStatus::Accepted,
       "N12 initial real WAV import");
 check(IntegrationAccess::paths(*f.c)[0]==aPath&&
       snapshot().names[0]=="A-good.wav"&&snapshot().wav[0]==wavA,
       "N12 initial sample/controller/project mismatch");
 check(f.c->stepSample(0,+1)==Controller::LoadStatus::Accepted&&
       IntegrationAccess::paths(*f.c)[0]==cPath&&
       snapshot().names[0]=="C-good.wav"&&snapshot().wav[0]==wavC,
       "N12 next did not skip invalid WAV and reach next valid sample");
 check(f.c->stepSample(0,-1)==Controller::LoadStatus::Accepted&&
       IntegrationAccess::paths(*f.c)[0]==aPath&&
       snapshot().names[0]=="A-good.wav"&&snapshot().wav[0]==wavA,
       "N12 previous did not skip invalid WAV");
 check(f.c->stepSample(0,-1)==Controller::LoadStatus::Accepted&&
       IntegrationAccess::paths(*f.c)[0]==cPath&&
       snapshot().wav[0]==wavC,
       "N12 previous did not wrap after invalid final file");
 check(f.c->stepSample(0,+1)==Controller::LoadStatus::Accepted&&
       IntegrationAccess::paths(*f.c)[0]==aPath&&
       snapshot().wav[0]==wavA,
       "N12 next did not wrap after invalid final file");
 // SOR02: restore both VST3 state streams and retain the original folder.
 ResizableMemoryIBStream componentState;
 ResizableMemoryIBStream controllerState;
 check(f.p->getState(&componentState)==kResultOk&&
       f.c->getState(&controllerState)==kResultOk,
       "SOR02 could not capture host project state");
 componentState.seek(0,IBStream::kIBSeekSet,nullptr);
 controllerState.seek(0,IBStream::kIBSeekSet,nullptr);
 check(f.c->setComponentState(&componentState)==kResultOk&&
       IntegrationAccess::paths(*f.c)[0]==aPath,
       "SOR02 matching kit recall discarded valid browsing origin");
 check(f.c->setState(&controllerState)==kResultOk&&
       IntegrationAccess::paths(*f.c)[0]==aPath,
       "SOR02 controller state did not restore browsing origin");
 check(f.c->stepSample(0,+1)==Controller::LoadStatus::Accepted&&
       IntegrationAccess::paths(*f.c)[0]==cPath,
       "SOR02 next sample failed after two-stream project recall");
 check(f.c->stepSample(0,-1)==Controller::LoadStatus::Accepted&&
       IntegrationAccess::paths(*f.c)[0]==aPath,
       "SOR02 previous sample failed after project recall");
 // Opposite legal host order: Controller state precedes component state.
 componentState.seek(0,IBStream::kIBSeekSet,nullptr);
 controllerState.seek(0,IBStream::kIBSeekSet,nullptr);
 check(f.c->setState(&controllerState)==kResultOk&&
       f.c->setComponentState(&componentState)==kResultOk&&
       IntegrationAccess::paths(*f.c)[0]==aPath,
       "SOR02 opposite restore ordering lost matching origin");
 check(f.c->stepSample(0,+1)==Controller::LoadStatus::Accepted&&
       IntegrationAccess::paths(*f.c)[0]==cPath,
       "SOR02 next failed when component state restored second");
 check(f.c->stepSample(0,-1)==Controller::LoadStatus::Accepted&&
       IntegrationAccess::paths(*f.c)[0]==aPath,
       "SOR02 return to original sample failed");

 const auto previous=snapshot();
 check(f.c->stepSample(0,0)==Controller::LoadStatus::Rejected&&
       f.c->stepSample(1,+1)==Controller::LoadStatus::NotFound&&
       snapshot().wav[0]==previous.wav[0]&&
       IntegrationAccess::paths(*f.c)[0]==aPath,
       "N12 invalid browsing request changed existing sample");
 std::error_code ec;std::filesystem::remove_all(folder,ec);
 check(!ec,"N12 unable to remove sample browse fixtures");
 f.p->setProcessing(false);f.p->setActive(false);
 evidence("N12 real sample folder browsing, invalid-neighbor skip, wrap, rollback PASS");
}
void testHostNameAllocationRollback(){
 Fixture f;auto baseline=namedKit('X');f.kit(baseline);
 // Construct the host message before fault injection: only the processor's
 // string allocation may fail. Names > SSO require dynamic storage.
 HostMessage rename;rename.setMessageID("Drumator.RenameRow");
 auto* attrs=rename.getAttributes();
 std::string longName(64,'N');
 check(attrs&&attrs->setInt("Row",0)==kResultOk&&
       attrs->setBinary("DisplayName",longName.data(),
         static_cast<uint32>(longName.size()))==kResultOk,
       "N10 rename host fixture");
 allocationFail=0;
 bool escaped=false;tresult result=kResultFalse;
 try {result=f.p->notify(&rename);}
 catch(const std::bad_alloc&) {escaped=true;}
 allocationFail=-1;
 check(!escaped&&result==kOutOfMemory,
       "N10 failed name allocation escaped VST3 notify or returned wrong result");
 auto readName=[&](){
   ResizableMemoryIBStream buffer;KitState state;
   check(f.p->getState(&buffer)==kResultOk&&
         KitCodec::decode(static_cast<const std::uint8_t*>(buffer.getData()),
                          static_cast<std::size_t>(buffer.getCursor()),state),
         "N10 save real processor name");
   return state.displayNames[0];
 };
 check(readName()==baseline.displayNames[0],
       "N10 rejected RenameRow damaged previous project label");
 check(f.p->notify(&rename)==kResultOk&&readName()==longName,
       "N10 valid RenameRow failed after allocation recovery");
 evidence("N10 processor rename fault injection and unchanged-state rollback PASS");
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
#include "FunctionalAuditCases.inl"
#include "SecondAuditCases.inl"
void testTransportSequenceTorture(){
 Fixture f;KitState kit;kit.visible=1;kit.grid.fill(0);
 kit.names[0]="transport.wav";kit.wav[0]=constantStereo(.25f,-.5f);
 f.kit(kit);prepareAudio(*f.p);
 AuditAudioBlock block;
 constexpr auto playing=ProcessContext::kPlaying|ProcessContext::kTempoValid|
                        ProcessContext::kProjectTimeMusicValid;
 EventList notes;Event hit{};hit.type=Event::kNoteOnEvent;
 hit.noteOn.pitch=36;hit.noteOn.velocity=1.f;hit.noteOn.noteId=-1;
 check(notes.addEvent(hit)==kResultOk,"TORTURE MIDI trigger fixture");
 unsigned stale=0,wrongSilence=0,missingHits=0;
 for(unsigned cycle=0;cycle<96;++cycle){
   // Empty pattern: every sound MUST originate from this explicit MIDI hit.
   // This detects a missed hit separately from a stale voice or double hit.
   block.data.numSamples=256;
   block.clock.state=playing;
   block.clock.projectTimeMusic=(cycle%2?4.:8.);
   block.data.inputEvents=&notes;
   check(f.p->process(block.data)==kResultOk,"TORTURE MIDI trigger block");
   // Verify sample-exact onset and prefix, not only an interior sample.
   // A delayed MIDI event or missing initial PCM must fail mutation QA.
   for(std::size_t sample=0;sample<128;++sample)
     if(std::abs(block.left[sample]-.25f)>1e-6f||
        std::abs(block.right[sample]+.5f)>1e-6f)++missingHits;
   block.data.inputEvents=nullptr;
   block.data.numSamples=(cycle%3==0?0:256);
   block.clock.state=0;
   check(f.p->process(block.data)==kResultOk,"TORTURE stopped callback");
   if(IntegrationAccess::activeAudioVoices(*f.p)!=0)++stale;
   block.data.numSamples=256;block.clock.state=playing;
   block.clock.projectTimeMusic=(cycle%4==0?4.:8.);
   check(f.p->process(block.data)==kResultOk,"TORTURE restart block");
   if(block.left[80]!=0.f||block.right[80]!=0.f)++wrongSilence;
   // Moving the host clock while playing must never resurrect old voices.
   block.clock.projectTimeMusic=(cycle%2?8.:4.);
   check(f.p->process(block.data)==kResultOk,"TORTURE locate/loop block");
   if(block.left[80]!=0.f||block.right[80]!=0.f)++wrongSilence;
 }
 qa::workflowNotice("TORTURE 96 MIDI/stop/restart/loop sequences: stale="+
   std::to_string(stale)+" missing-or-doubled-hits="+std::to_string(missingHits)+
   " unexpected-PCM="+std::to_string(wrongSilence));
 f.p->setProcessing(false);f.p->setActive(false);
 check(stale==0&&missingHits==0&&wrongSilence==0,
       "TORTURE missed/doubled trigger, stale voice or unexpected output");
}
void testPacedSampleSwapContinuity(){
 using Clock=std::chrono::steady_clock;
 Fixture active,baseline;
 KitState kit;kit.grid.fill(0);kit.visible=2;
 kit.names[1]="reference.wav";kit.wav[1]=constantStereo(.25f,-.5f);
 active.kit(kit);baseline.kit(kit);
 prepareAudio(*active.p);prepareAudio(*baseline.p);
 const auto folder=std::filesystem::current_path()/"125A-paced-import-QA";
 std::filesystem::create_directories(folder);
 const auto fileA=folder/"swapA.wav",fileB=folder/"swapB.wav";
 for(const auto& [path,left,right]:std::array<std::tuple<std::filesystem::path,float,float>,2>{
       std::tuple{fileA,.125f,-.25f},std::tuple{fileB,-.375f,.125f}}){
   const auto wav=constantStereo(left,right);
   std::ofstream output(path,std::ios::binary|std::ios::trunc);
   output.write(reinterpret_cast<const char*>(wav.data()),
                static_cast<std::streamsize>(wav.size()));
   check(output.good(),"PACING fixture write");
 }
 std::atomic<unsigned> blocks{0},badPcm{0},errors{0},misses{0};
 std::atomic<double> maximumMs{0.};
 // Audio actor owns writes; read only after join. Fixed memory, no
 // allocations in the callback or measured pacing loop.
 std::array<double,500> callbackMs{};
 std::jthread actor([&](std::stop_token stop){
   AuditAudioBlock tested,reference;
   EventList events;Event note{};note.type=Event::kNoteOnEvent;
   note.noteOn.pitch=37;note.noteOn.velocity=1.f;note.noteOn.noteId=-1;
   if(events.addEvent(note)!=kResultOk){++errors;return;}
   tested.data.inputEvents=&events;reference.data.inputEvents=&events;
   auto target=Clock::now();
   for(unsigned n=0;n<500&&!stop.stop_requested();++n){
     const auto begin=Clock::now();
     const auto ok=active.p->process(tested.data);
     const auto end=Clock::now();
     const double ms=std::chrono::duration<double,std::milli>(end-begin).count();
     callbackMs[n]=ms;
     if(ms>256./48000.*1000.)++misses;
     auto peak=maximumMs.load();
     while(ms>peak&&!maximumMs.compare_exchange_weak(peak,ms)){}
     if(ok!=kResultOk||baseline.p->process(reference.data)!=kResultOk)++errors;
     for(std::size_t i=0;i<256;++i)
       if(tested.left[i]!=reference.left[i]||tested.right[i]!=reference.right[i])
         ++badPcm;
     ++blocks;
     target+=std::chrono::duration_cast<Clock::duration>(
                  std::chrono::duration<double>(256./48000.));
     std::this_thread::sleep_until(target);
   }
 });
 while(blocks.load()<4&&errors.load()==0)std::this_thread::yield();
 for(unsigned i=0;i<12;++i){
   const auto path=(i&1)?fileA:fileB;
   if(active.c->loadWav(0,pathToUtf8(path))!=Controller::LoadStatus::Accepted)
     ++errors;
 }
 actor.join();
 // Assert that accepted controller imports actually reach the processor kit.
 // Unchanged-row PCM alone cannot distinguish an ignored import from success.
 ResizableMemoryIBStream importedState;
 KitState importedKit;
 check(active.p->getState(&importedState)==kResultOk&&
       KitCodec::decode(static_cast<const std::uint8_t*>(importedState.getData()),
                        static_cast<std::size_t>(importedState.getCursor()),importedKit),
       "PACING imported kit state decode");
 check(importedKit.wav[0]==constantStereo(.125f,-.25f),
       "PACING accepted sample import did not update processor asset");
 // Compute quantiles off the actor thread. An otherwise green PCM
 // comparison must never hide measured audio callback overruns.
 auto sortedCallbackMs=callbackMs;
 std::sort(sortedCallbackMs.begin(),sortedCallbackMs.end());
 const double p95=sortedCallbackMs[474],p99=sortedCallbackMs[494];
 qa::workflowNotice("PACING 48k/256 blocks="+std::to_string(blocks.load())+
   " unchanged-row PCM mismatches="+std::to_string(badPcm.load())+
   " deadline-misses="+std::to_string(misses.load())+
   " p95-ms="+std::to_string(p95)+" p99-ms="+std::to_string(p99)+
   " max-callback-ms="+std::to_string(maximumMs.load())+
   "; paced shared Windows runner, NOT Studio One/ASIO certification");
 std::error_code ec;std::filesystem::remove_all(folder,ec);
 active.p->setProcessing(false);active.p->setActive(false);
 baseline.p->setProcessing(false);baseline.p->setActive(false);
 check(blocks.load()==500&&badPcm.load()==0&&errors.load()==0,
       "PACING actual-controller sample swaps disrupted unchanged audio");
 check(misses.load()==0,
       "PACING missed an audio deadline despite a previously green PCM result");
}
void testComponentOnlySameBasenameRejectsStaleOrigin(){
 Fixture f;
 KitState initial;initial.grid.fill(0);initial.visible=1;
 f.kit(initial);prepareAudio(*f.p);
 const auto directory=std::filesystem::current_path()/"125A-v1a04-source";
 std::filesystem::create_directories(directory);
 const auto source=directory/"kick.wav";
 {
   const auto bytes=constantStereo(.25f,-.5f);
   std::ofstream out(source,std::ios::binary);
   out.write(reinterpret_cast<const char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
   check(out.good(),"V1A04 write source WAV");
 }
 check(f.c->loadWav(0,pathToUtf8(source))==Controller::LoadStatus::Accepted,
       "V1A04 load source via real controller");
 check(!IntegrationAccess::paths(*f.c)[0].empty(),
       "V1A04 expected original navigation path");
 KitState other;other.grid.fill(0);other.visible=1;
 other.names[0]="kick.wav";other.wav[0]=constantStereo(-.125f,.375f);
 const auto state=Fixture::encode(other);
 check(f.recall(state)==kResultOk,"V1A04 component-only different kit recall");
 check(IntegrationAccess::paths(*f.c)[0].empty(),
       "V1A04 component-only recall retained stale same-basename origin");
 std::error_code ec;std::filesystem::remove_all(directory,ec);
}
void testPeerVelocityCancelPreservesCommittedStep(){
 Fixture f;KitState kit;kit.grid.fill(0);kit.grid[0]=60;f.kit(kit);
 f.open();f.open();
 auto* first=IntegrationAccess::view(*f.c,0);
 auto* peer=IntegrationAccess::view(*f.c,1);
 click(*first,1248.,UiCompactLayout::header+88.);
 click(*first,630.,UiCompactLayout::header+UiCompactLayout::baseRow+12.);
 check(std::get<0>(IntegrationAccess::viewSnapshot(*first)),
       "V1A03 first view gesture absent");
 click(*peer,630.,UiCompactLayout::header+UiCompactLayout::baseRow+88.);
 check(f.c->velocity(0,1,0)==100,
       "V1A03 peer step was not committed");
 // The peer edit commits the first gesture, so its last velocity
 // must survive a subsequently delivered stale Cancel unchanged.
 const auto committedOwnVelocity=f.c->velocity(0,0,0);
 first->onMouseCancel();
 check(f.c->velocity(0,1,0)==100,
       "V1A03 late Cancel erased second view step");
 check(f.c->velocity(0,0,0)==committedOwnVelocity,
       "V1A03 stale Cancel changed previously committed velocity");
}
void testZeroFrameStopRetiresVoice(){
 Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
 kit.names[0]="zero-stop.wav";kit.wav[0]=constantStereo(.25f,-.5f);
 f.kit(kit);prepareAudio(*f.p);
 AuditAudioBlock audio;
 constexpr auto playing=ProcessContext::kPlaying|ProcessContext::kTempoValid|
                        ProcessContext::kProjectTimeMusicValid;
 audio.clock.state=playing;audio.clock.projectTimeMusic=4.;
 EventList notes;Event hit{};hit.type=Event::kNoteOnEvent;hit.noteOn.pitch=36;
 hit.noteOn.velocity=1.f;hit.noteOn.noteId=-1;
 check(notes.addEvent(hit)==kResultOk,"SOR03Z trigger fixture");
 audio.data.inputEvents=&notes;
 check(f.p->process(audio.data)==kResultOk&&IntegrationAccess::activeAudioVoices(*f.p)==1,
       "SOR03Z initial voice missing");
 audio.data.inputEvents=nullptr;audio.data.numSamples=0;
 audio.clock.state=0;audio.clock.projectTimeMusic=4.+256./24000.;
 check(f.p->process(audio.data)==kResultOk&&IntegrationAccess::activeAudioVoices(*f.p)==0,
       "SOR03Z zero-frame stopped callback failed to retire voice");
 audio.data.numSamples=256;audio.clock.state=playing;
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==0.f&&audio.right[80]==0.f,
       "SOR03Z continuous PPQ restart resumed old voice");
 f.p->setProcessing(false);f.p->setActive(false);
}
void testTransportJumpRetiresOldVoices(){
 Fixture f;
 KitState kit;kit.visible=1;kit.grid.fill(0);kit.grid[0]=100;
 kit.names[0]="restart.wav";kit.wav[0]=constantStereo(.25f,.25f);
 f.kit(kit);prepareAudio(*f.p);
 AuditAudioBlock block;
 block.clock.state=ProcessContext::kPlaying|
       ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 block.clock.projectTimeMusic=4.0;
 check(f.p->process(block.data)==kResultOk,"SOR03 first transport block");
 check(IntegrationAccess::activeAudioVoices(*f.p)==1,
       "SOR03 first voice absent");
 // Simulate a host which delivers no stopped transport process callback.
 // It locates to bar three, preserving playing=true on both observed blocks.
 block.clock.projectTimeMusic=8.0;
 check(f.p->process(block.data)==kResultOk,"SOR03 host jump process");
 check(IntegrationAccess::activeAudioVoices(*f.p)==1,
       "SOR03 stale previous-run voice survived host locate");
 // Render the full callback, not only a voice counter. A loop discontinuity
 // must produce a valid finite new hit; stale voices must not double the PCM.
 check(std::isfinite(block.left[80])&&std::isfinite(block.right[80])&&
       std::abs(block.left[80])<=.25f&&std::abs(block.right[80])<=.25f,
       "SOR03 loop jump produced doubled or nonfinite sample audio");
 f.p->setProcessing(false);f.p->setActive(false);
 evidence("SOR03 native processor timeline jump retires old voice without stopped callback PASS");
}
#include "ReauditCases.inl"
#include "FinalAudit20261023.inl"
}
int main(int argc,char** argv){
 std::cout<<std::unitbuf;
 if(argc!=2&&argc!=3)return 2;
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 moduleHandle=GetModuleHandleW(nullptr);VSTGUI::init(static_cast<HINSTANCE>(moduleHandle));
 auto* platform=VSTGUI::getPlatformFactory().asWin32Factory();
 if(!platform)return 3;platform->setResourceBasePath(VSTGUI::UTF8String(argv[1]));
 entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 if(argc==3){
   const bool warm=std::strcmp(argv[2],"--reaudit-song-warm")==0;
   if(!warm&&std::strcmp(argv[2],"--reaudit-song-cold")!=0)return 2;
   int childResult=0;
   try{testReauditSongCold(warm);}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';childResult=3;}
   CloseHandle(entered);CloseHandle(resume);VSTGUI::exit();CoUninitialize();return childResult;
 }

 int failures=0;
 std::string auditDiagnostics;drumator::qa::collectedNotices=&auditDiagnostics;
 std::string qaCaseSummary="Executed native integration cases:\n";
 auto run=[&](const std::string& name,const auto& test){
   try{test();const auto line=name+" PASS";evidence(line);qaCaseSummary+=line+'\n';}
   catch(const std::exception& ex){allocationFail=-1;releaseProbe();++failures;
     const auto line=name+" FAIL: "+ex.what();evidence(line);qaCaseSummary+=line+'\n';}
 };
 run("RE01 original deterministic VFA02",testReauditPatternLength);
 run("RE02 original fresh DZM3 matrix",testFinalAuditDzm3);
 run("RE03 paired duration fallback in Pattern and Song",testReauditDurationFallback);
 run("RE04 peer FILL survives late Cancel",testReauditPeerFill);
 run("RE05 component-first recall does not arm next kit",testReauditOriginOrder);
 run("RE06 reentrant setDirty captures consistent streams",testReauditDirtySave);
 run("RE07 host pattern feedback before component recall",testReauditHostMirror);
 run("RE08 independent complete PCM timeline",testReauditPcmTimeline);
 run("RE09 64 voices live swap locate Stop",testReauditLoadedSwap);
 run("RE10 actual Processor deadline matrix",testReauditDeadlineMatrix);
 run("A26 combined 16 active buses, 64 voices, 12 imports, Cycle Stop and PCM",testA26CombinedLiveMultibusCycleImport);
 run("RE11 cold Song overlap cannot divide by zero",[&]{testReauditSongColdChild(argv[1]);});
 run("RM01 zero Stop repair mutation",[]{expectMutationKilled(qa::AuditMutation::suppressZeroStop,"zero Stop",testZeroFrameStopRetiresVoice);});
 run("RM02 paired duration repair mutation",[]{expectMutationKilled(qa::AuditMutation::staleDuration,"paired duration",testReauditPatternLength);});
 run("RM03 peer owner repair mutation",[]{expectMutationKilled(qa::AuditMutation::skipPeerCommit,"peer commit",testPeerVelocityCancelPreservesCommittedStep);});
 run("RM04 asset identity repair mutation",[]{expectMutationKilled(qa::AuditMutation::basenameOnly,"asset identity",testComponentOnlySameBasenameRejectsStaleOrigin);});
 run("RM05 TORTURE missing MIDI mutation",[]{expectMutationKilled(qa::AuditMutation::dropMidi,"TORTURE missing MIDI",testTransportSequenceTorture);});
 run("RM06 TORTURE duplicate MIDI mutation",[]{expectMutationKilled(qa::AuditMutation::doubleMidi,"TORTURE doubled MIDI",testTransportSequenceTorture);});
 run("RM07 TORTURE one-sample delay mutation",[]{expectMutationKilled(qa::AuditMutation::delayMidiOne,"TORTURE one-sample delay",testTransportSequenceTorture);});
 run("RM08 TORTURE missing prefix mutation",[]{expectMutationKilled(qa::AuditMutation::zeroPrefix,"TORTURE zero first32",testTransportSequenceTorture);});
 run("RM09 PACING accepted import no-op mutation",[]{expectMutationKilled(qa::AuditMutation::skipImport,"PACING import no-op",testPacedSampleSwapContinuity);});
 run("RM10 PACING active dropout mutation",[]{expectMutationKilled(qa::AuditMutation::muteImportedProcessor,"PACING active-only dropout",testPacedSampleSwapContinuity);});
 run("RM11 independent PCM detects offset and gaps",testReauditPcmNegatives);
 run("RM12 PACING deadline diagnostic injection",testReauditDeadlineDiagnostic);
 run("RM13 loaded swap detects ignored replacement",[]{expectMutationKilled(qa::AuditMutation::skipImport,"64 voice audible replacement",testReauditLoadedSwap);});
 run("WE01 actual WAV preview and project recall",testWavEditorPreviewAndRecall);
 run("T03 controlled actual bank",testPatternOverlap);
 run("T04 controlled real restore gate",testKitGate);
 for(bool zero:{true,false})for(bool busy:{false,true})for(const auto& points:std::vector<std::vector<unsigned>>{{5},{2},{5,2}}){
   const std::string name="O01 "+std::string(zero?"zero":"audio")+(busy?" busy ":" snapshot ")+
     (points.size()>1?"ABA":points[0]==2?"equal":"different");
   run(name,[&]{testArbitration(zero,busy,points);});
 }
 run("Z01 full Controller allocation injection",testRecallOOM);
 run("Z01 legacy Controller",testLegacyReject);
 run("V18 multi-view native lifecycle",testMultiViewLifecycleNotifications);
 run("Z02 native attached views",testZoom);
 run("VSTGUI precision controls",testPrecisionPanel);
 run("VSTGUI native pixel render set",testVisualRender);
 run("GUI49 musical four-sample native 130pct capture",testGui49MusicalFourSampleCapture);
 run("GUI 11I centered PATTERN and meter header geometry",test11IHeaderPatternCentered);
 run("M11I actual MAIN and stem meter, GUI capture and clip reset",test11IMeterStereoAndStems);
 run("M11I normal 16bit mono FLAC import",test11IMonoFlacImport);
 run("M11I six rotary Min Center Max and double-click defaults",test11IRotaryMinCenterMax);
 run("VSTGUI velocity design capture 100/130",testVelocityDesignCaptures);
 run("VSTGUI wave editor design capture 100/130",testWaveEditorDesignCaptures);
 run("W11D native mouse-driven wave editor controls",testWaveEditorRealMouseControls);
 run("W11F real WAV/FLAC imports and status",testWaveEditor11FFlacAndStatus);
 run("W11F independent stereo fade PCM",testWaveEditor11FStereoFadePcm);
 run("W11F mouse-scroll source PCM numeric oracle",testWaveEditor11FScrollNumeric);
 run("VSTGUI long WAV display full/zoom at 100/130",testWaveEditorLongCaptures);
  run("GUI physical step state/raster toggle",testGuiPhysicalStepsRaster);
 run("UX direct pattern length",testPatternLengthControls);
 run("P18 native pattern duplicate menu",testPatternDuplicateMenu);
 run("P19 1T/2T copy-paste-duplicate duration",testPatternDurationClipboard);
 run("T28 rate-switch transport clock reset",testRateChangeTransportReset);
 run("SOR03 host timeline jump voice cleanup",testTransportJumpRetiresOldVoices);
 run("SOR03Z zero-frame stop retires old voices",testZeroFrameStopRetiresVoice);
 run("V1A03 peer edit survives velocity cancel",testPeerVelocityCancelPreservesCommittedStep);
 run("V1A04 component-only same-name origin safety",testComponentOnlySameBasenameRejectsStaleOrigin);
 run("PACING long running sample swap audio continuity",testPacedSampleSwapContinuity);
 run("TORTURE 96 transport stop and locate sequences",testTransportSequenceTorture);
 run("RANDOM one-shot existing notes",testRandomVelocityOneShot);
  run("SOR01 new Step Velocity 100",testNewStepDefault100);
 run("VELO 100 reset existing notes",testVelocityResetPattern);
 run("V16 1T hidden velocity preservation",testVelocityInactiveBarProtection);
 run("V17 1T row FILL/CLEAR hidden-step protection",testFillActivePatternLength);
 run("UX Velocity empty-step editing",testVelocityLaneExistingStepsOnly);
 run("UX paged Song board",testPagedSongEditor);
 run("S09 Song entry signed extreme edit deltas",testSongEntryExtremeDelta);
 run("S10 Song count signed extreme edit deltas",testSongCountExtremeDelta);
 run("D11 control signed extreme deltas",testControlExtremeDeltas);
 run("UX compact Song viewport",testSongCompactViewport);
 run("N10 processor rename OOM",testHostNameAllocationRollback);
 run("N11 controller metadata transactional commit",testControllerMetadataCommit);
 run("N12 real WAV sample folder previous/next",testRealSampleFolderStepping);
 run("Z03 Win32 name editor",testNativeName);
 run("FA02 MIDI/host arbitration during actual kit restore",testAuditMidiRecall);
 run("FA02 zero-frame valid-offset arbitration",testAuditZeroFlushOffsets);
 run("FA03 new-kit Song fallback during publication",testAuditSongGeneration);
 run("FA04 sample path failures and real View drop callbacks",testAuditSamplePathFailures);
 run("FA05 modal menu gutters",testAuditMenuGutters);
 run("FA06 minimum-height host refusal menus",testAuditMinimumHeightMenus);
 run("SA01 two-view native invalidation after row and global edits",testSecondAuditViewInvalidation);
 run("SA02 actual sequencer/MIDI choke partition",testSecondAuditTriggerPartition);
 run("FA23-01 edited Clear remains saveable",testFa23ClearEditedState);
 run("FA23-02 preview restarts without layering",testFa23PreviewRetrigger);
 run("FA23-03 preview publication boundary",testFa23PreviewPublication);
 run("FA23-04 WAV modal blocks drop",testFa23ModalDrop);
 run("FA23-05 stale draft cannot edit new sample",testFa23StaleEditor);
 run("FA23-06 multiple restore provenance cycles",testFa23OriginCycles);
 run("FA23-07 Undo duration during live gesture",testFa23UndoDuringStroke);
 run("FA23-08 interpolation respects trim bounds",testFa23TrimInterpolation);
 run("FA23-09 audition deactivation",testFa23AuditionDeactivate);
 run("FA23-10 waveform query OOM",testFa23QueryOOM);
 run("FR01 stop ordering after consumed start",testFr01StopAfterStartConsumed);
 run("RV01 stop then new preview ordering",testRv01RestartAfterStopDuringAudioStart);
 run("RV01 asset+command publication ordering",testRv01AssetCommandPublication);
 run("RV01 preview retirement across outputless callback",testRv01OutputlessPreviewRetirement);
 run("A04 preview lifecycle across 48k / 44.1k / 48k",testA04PreviewRateLifecycle);
 run("A05 dense Song automation one schedule",testA05DenseSongAutomationDoesNotReschedule);
 run("A06 committed WAV remains accepted after repaint OOM",testA06CommittedImportSurvivesRepaintFailure);
 run("A07 failed peer view refresh preserves accepted edit and dirty request",testA07NotificationFaultIsolation);
 run("A08 native WAV editor revision cache and invalidation",testA08WaveEditorRevisionCache);
 run("RM16 waveform cache disabled is detected",[]{expectMutationKilled(qa::AuditMutation::disableWaveCache,"full WAV on every viewport query",testA08WaveEditorRevisionCache);});
 run("A09 native variable stereo waveform exact reference",testA09WaveformExtremaAgainstNativeReference);
 run("A10 context gap audio and step continuity",testA10MissingContextAudioContinuity);
 run("A11 verified Cycle tail vs Locate",testA11CycleWrapPreservesSampleTails);
 run("A12 nonaligned Cycle wrap and negative controls",testA12CycleWrapMidBlockAndNegativeControls);
 run("A13 internal Step at mid-block Cycle",testA13PureSequencerCyclePcm);
 run("A14 pattern and Song at intra-block Cycle seam",testA14PatternAndSongAcrossCycleSeam);
 run("A15 Cycle random velocity block-invariance",testA15CycleRandomVelocityReproducible);
 run("A16 multiple Cycles in one host callback",testA16MultiCyclePerCallback);
 run("A17 optional Cycle context gap full stereo PCM",testA17MissingCycleClockKeepsExactPcm);
 run("A18 variable sample rate and host frames Cycle PCM",testA18CycleBlockPartitionAcrossSampleRates);
 run("A19 changing tempo within active Cycle PCM",testA19TempoChangeWithinHostCycle);
 run("A20 partial clock explicitly disables Cycle",testA20PartialClockExplicitCycleDisable);
 run("A21 valid tempo with temporarily absent PPQ",testA21PartialTempoWithMissingPpq);
 run("A22 bounded Song snapshot per host callback",testA22SongSnapshotBoundedByHostCallbacks);
 run("A23 valid host PPQ during missing tempo and locate",testA23AuthoritativePpqSurvivesMissingTempo);
 run("A24 component-first DZM3 origin permissions are one-shot",testA24ComponentFirstDzm3OriginConsumed);
 run("A25 old unpaired Component then new DZM3 keeps provenance",testA25UnpairedComponentNewDzm3Origin);
 run("RM28 dropping valid partial PPQ is detected",[]{expectMutationKilled(qa::AuditMutation::ignoreValidPartialPpq,"host PPQ lost with invalid tempo",testA23AuthoritativePpqSurvivesMissingTempo);});
 run("RM29 stale component-first DZM3 permission detected",[]{expectMutationKilled(qa::AuditMutation::keepComponentFirstOriginArmed,"component-first DZM3 origin leak",testA24ComponentFirstDzm3OriginConsumed);});
 run("RM27 repeated per-Cycle Song snapshot detected",[]{expectMutationKilled(qa::AuditMutation::repeatSongSnapshots,"Song re-snapshot on every Cycle",testA22SongSnapshotBoundedByHostCallbacks);});
 run("RM25 stale Cycle after explicit partial disable killed",[]{expectMutationKilled(qa::AuditMutation::keepCycleOnPartialDisable,"partial ProcessContext Cycle-off ignored",testA20PartialClockExplicitCycleDisable);});
 run("RM26 valid partial tempo ignored killed",[]{expectMutationKilled(qa::AuditMutation::ignorePartialTempo,"partially valid host BPM ignored",testA21PartialTempoWithMissingPpq);});
 run("RM23 variable block no Cycle split is detected",[]{expectMutationKilled(qa::AuditMutation::skipCycleSplit,"variable callback Cycle split",testA18CycleBlockPartitionAcrossSampleRates);});
 run("RM24 stale Cycle tempo mutation is detected",[]{expectMutationKilled(qa::AuditMutation::freezeCycleTempo,"wrong tempo at Cycle seam",testA19TempoChangeWithinHostCycle);});
 run("RM22 missing Cycle clock cache is killed",[]{expectMutationKilled(qa::AuditMutation::suppressCycleClockCache,"missing Cycle clock caused skipped sample triggers",testA17MissingCycleClockKeepsExactPcm);});
 run("RM19 old unsplit Cycle scheduler is killed",[]{expectMutationKilled(qa::AuditMutation::skipCycleSplit,"unsplit host Cycle lost Step",testA13PureSequencerCyclePcm);});
 run("RM20 old linear Velocity PPQ is killed",[]{expectMutationKilled(qa::AuditMutation::skipCycleVelocityMap,"randomized loop velocity changed with block partition",testA15CycleRandomVelocityReproducible);});
 run("RM21 old single-wrap voice reset is killed",[]{expectMutationKilled(qa::AuditMutation::skipCycleModulo,"multiple Cycle wraps retire valid tails",testA16MultiCyclePerCallback);});
 run("RM18 missing-context old-cutoff regression is killed",[]{expectMutationKilled(qa::AuditMutation::suppressMissingContextCarry,"missing clock truncates hits",testA10MissingContextAudioContinuity);});
 run("RM17 direct-scan mutation is detected",[]{expectMutationKilled(qa::AuditMutation::bypassWaveRangeIndex,"linear scan instead of indexed bins",testA09WaveformExtremaAgainstNativeReference);});
 run("RV02 allocation-free STOP and rejected-delivery retry",testRv02AllocationFreeStopAndRetry);
 run("RV02 transient STOP rejection during native editor close",testRv02NativeEditorCloseTransientReject);
 run("RV02 rejected STOP leaves Apply uncommitted",testRv02ApplyRejectKeepsEdit);
 run("A03 permanent STOP refusal and recovery",testA03PersistentStopRejectionRecovery);
 run("R05 sample menu allocation failures",testR05SampleMenuAllocationFailures);
 run("FR02 controller query/edit OOM boundaries",testFr02ControllerMessageOOM);
 run("RV03 full and legacy recall query allocation failures",testRv03RecallQueryOOM);
 run("FR03 legacy sample-present synchronization",testFr03LegacyRecallSamplePresence);
 run("FR08 controller path length consistency",testFr08PathSerializationBoundary);
 run("FR04 waveform border real mouse input",testFr04RealWaveformEdgeMouse);
 run("FR05 one-frame Fade Out native pixel contract",testFr05ShortFadePixelsMatchAudioContract);
 run("FR06 modal error message native pixels",testFr06ModalErrorPixels);
 run("FR10 global knob right-edge mouse input",testFr10GlobalKnobEdgeHit);
 run("RV05 all global knob rendered centers",testRv05GlobalKnobPaintCenters);
 run("RM14 historical Fade renderer regression killed",[]{
   expectMutationKilled(qa::AuditMutation::oldFadeRenderer,"RV04 historical Fade/Gain formula",
                        testFr05ShortFadePixelsMatchAudioContract);
 });
 run("RM15 historical knob draw offsets killed",[]{
   expectMutationKilled(qa::AuditMutation::oldKnobDrawCenters,"RV05 original 80/178/1178 draw centers",
                        testRv05GlobalKnobPaintCenters);
 });
 evidence("Integration total failures: "+std::to_string(failures)+"; Studio One NOT RUN");
 qaCaseSummary+="Total failures: "+std::to_string(failures)+"; Studio One NOT RUN";
 drumator::qa::collectedNotices=nullptr;
 drumator::qa::workflowNotice(qaCaseSummary+"\nDetailed audit diagnostics:\n"+auditDiagnostics);
 CloseHandle(entered);CloseHandle(resume);VSTGUI::exit();CoUninitialize();return failures?1:0;
}
