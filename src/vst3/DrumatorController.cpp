#include "DrumatorController.h"
#include "DrumatorView.h"
#include "DrumatorEditor.h"
#include "PluginIDs.h"
#include "core/WavPcm16Decoder.h"
#include "core/FlacImport.h"
#include "core/StepVelocity.h"
#include "core/HostEditDirty.h"
#include "core/PatternQuantization.h"
#include "core/PreparedKitNames.h"
#include "core/ZoomRecallPolicy.h"
#include "core/Utf8Path.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstattributes.h"
#include "vstgui/lib/cstring.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <exception>
#include "core/SampleFolderNavigation.h"
#include <fstream>
#include <vector>

namespace drumator {
void Controller::requestMeterSnapshot(){
 // Called by DrumatorView::pollMeter() noexcept. No allocation/host
 // exception may escape the VSTGUI timer or Windows host callback.
 Steinberg::Vst::IMessage* request=nullptr;
 try {
   request=allocateMessage();
   if(request){
     request->setMessageID("Drumator.MeterRequest");
     (void)sendMessage(request); // control actor; no host dirty flag
   }
 }catch(...) { /* retain the prior meter snapshot; UI fades naturally */ }
 if(request)request->release();
}
bool Controller::clearMeterClips(){
 Steinberg::Vst::IMessage* clear=nullptr;
 bool accepted=false;
 try {
   clear=allocateMessage();
   if(clear){
     clear->setMessageID("Drumator.MeterClear");
     accepted=sendMessage(clear)==Steinberg::kResultOk;
   }
 }catch(...) {accepted=false;}
 if(clear)clear->release();
 if(accepted){
   meterState_.clip=0;meterState_.full=0;
   ++lastClearEpoch_;
 }
 return accepted;
}
Steinberg::tresult PLUGIN_API Controller::notify(Steinberg::Vst::IMessage* message){
 if(message&&message->getMessageID()&&
    std::strcmp(message->getMessageID(),"Drumator.MeterSnapshot")==0){
   // The host may deliver IMessage across a process boundary. Treat getter
   // and attribute decoding as a non-throwing VST3 callback boundary.
   try {
   auto* attrs=message->getAttributes();
   Steinberg::int64 seq=-1,l=-1,r=-1,clip=-1,full=-1,reset=-1;
   if(!attrs||attrs->getInt("Sequence",seq)!=Steinberg::kResultOk||
      attrs->getInt("Left",l)!=Steinberg::kResultOk||
      attrs->getInt("Right",r)!=Steinberg::kResultOk||
      attrs->getInt("Clip",clip)!=Steinberg::kResultOk||
      attrs->getInt("Full",full)!=Steinberg::kResultOk||
      attrs->getInt("Reset",reset)!=Steinberg::kResultOk||
      seq<0||l<0||r<0||l>65535||r>65535||
      clip<0||full<0||clip>65535||full>65535||
      reset<0||static_cast<std::uint64_t>(reset)<lastClearEpoch_)
     return Steinberg::kResultFalse;
   const auto index=static_cast<std::uint64_t>(seq);
   // Later UI polls for the same audio generation must not erase peaks.
   if(index>meterState_.sequence){
     meterState_={index,static_cast<std::uint32_t>(l),static_cast<std::uint32_t>(r),
                  static_cast<std::uint32_t>(clip),static_cast<std::uint32_t>(full),
                  static_cast<std::uint32_t>(reset)};
     lastClearEpoch_=std::max(lastClearEpoch_,meterState_.reset);
   }
   return Steinberg::kResultOk;
   }catch(...) {return Steinberg::kOutOfMemory;}
 }

 if(message&&message->getMessageID()&&
    std::strcmp(message->getMessageID(),"Drumator.OutputMeterLink")==0){
    Steinberg::int64 id=0;
    auto* attr=message->getAttributes();
    if(!attr||attr->getInt("MeterId",id)!=Steinberg::kResultOk||id<=0)
      return Steinberg::kResultFalse;
    meter_=findMeter(static_cast<std::uint64_t>(id));
    // The registry is deliberately optional: distinct host processes have
    // separate address spaces and receive levels via MeterRequest/Snapshot.
    return Steinberg::kResultOk;
 }
 return EditController::notify(message);
}
Controller::~Controller() {
 if(reservedPreviewStop_) {
   reservedPreviewStop_->release();
   reservedPreviewStop_=nullptr;
 }
}
bool Controller::reservePreviewStop() noexcept {
 if(reservedPreviewStop_)return true;
 Steinberg::Vst::IMessage* candidate=nullptr;
 try {
   candidate=allocateMessage();
   if(!candidate)return false;
   candidate->setMessageID("Drumator.EditorPreviewStop");
   reservedPreviewStop_=candidate;
   return true;
 }catch(const std::bad_alloc&) {
   if(candidate)candidate->release();
   return false;
 }
}
// FNV-1a used only for control-side asset identity (not a security hash).
static std::uint64_t wavIdentity(const std::vector<std::uint8_t>& data) noexcept {
 std::uint64_t h=14695981039346656037ull;
 for(const auto byte:data){h^=byte;h*=1099511628211ull;}
 return h;
}
// Maintain pre-existing saved host automation thresholds while exposing
// coherent A-H text and normalized-to-plain mapping.
class LegacyPatternParameter final : public Steinberg::Vst::StringListParameter {
public:
 using Steinberg::Vst::StringListParameter::StringListParameter;
 Steinberg::Vst::ParamValue toPlain(Steinberg::Vst::ParamValue n) const override {
   return static_cast<Steinberg::Vst::ParamValue>(patternFromNormalized(n));
 }
 void toString(Steinberg::Vst::ParamValue n,Steinberg::Vst::String128 out) const override {
   const auto index=patternFromNormalized(n);
   const auto* label=strings[index];
   std::size_t i=0;
   while(i<127&&label[i]){out[i]=label[i];++i;}
   out[i]=0;
 }
};
void Controller::registerView(DrumatorView* view) {
 if(view)views_.push_back(view);
}
void Controller::unregisterView(DrumatorView* view) {
 views_.erase(std::remove(views_.begin(),views_.end(),view),views_.end());
 if(views_.empty()){
   editorWaveCache_=DecodedWav{};
   editorWaveIndexLeft_.clear();
   editorWaveIndexRight_.clear();
   editorWaveRow_=kRows;
 }
}
void Controller::notifyViews(bool relayout) {
 // After sendEditMessage() succeeded, the Processor has already committed
 // the state. A rejecting/throwing host dirty callback or a failed View
 // refresh must NEVER make the user operation look rejected or crash the host.
 // Keep the dirty request pending until it is acknowledged by the host.
 if(dirtyPending_&&componentHandler2) {
   try {
     if(componentHandler2->setDirty(true)==Steinberg::kResultOk)
       dirtyPending_=false;
   }catch(...) { /* retry on the next host/UI notification */ }
 }
 for(auto* view:views_)if(view) {
   try {view->refreshFromHost(relayout);}
   catch(...) { /* one failed View must not invalidate an accepted edit */ }
 }
}
void Controller::invalidateTransientEditsForRecall() noexcept {
 strokePattern_=kPatterns;
 strokeGeneration_=0;
 editGeneration_.onSuccessfulRecall();
 for(auto* view:views_)if(view)view->discardTransientEditsForRecall();
}
Steinberg::tresult PLUGIN_API Controller::setParamNormalized(
    Steinberg::Vst::ParamID id,Steinberg::Vst::ParamValue value) {
 const auto result=EditController::setParamNormalized(id,value);
 if(result==Steinberg::kResultOk&&id==kPatternParam) {
   // Keep a newer host pattern callback when Component-state recall arrives late.
   // Internal recall bypasses this override and does not set the pending flag.
   hostPatternPending_=true;
   hostPatternValue_=getParamNormalized(kPatternParam);
   notifyViews(false);
 }
 return result;
}
bool Controller::processorConfirmsPattern(unsigned requested) {
 Steinberg::Vst::IMessage* msg=nullptr;
 try {
   msg=allocateMessage();
   if(!msg)return false;
   // Host allocation and message dispatch can fail even after recall commits.
   // Always release the message and fall back to the recalled pattern.
   struct MessageOwner {
     Steinberg::Vst::IMessage* value;
     ~MessageOwner(){if(value)value->release();}
   } owner{msg};
   msg->setMessageID("Drumator.QueryPattern");
   const auto result=sendMessage(msg);
   Steinberg::int64 selected=-1;
   auto* attrs=msg->getAttributes();
   return result==Steinberg::kResultOk&&attrs&&
       attrs->getInt("Selected",selected)==Steinberg::kResultOk&&
       selected==static_cast<Steinberg::int64>(requested);
 }catch(const std::bad_alloc&) {
   // Only allocation failure before owner construction needs manual cleanup.
   // A post-commit query failure must not escape the VST3 recall callback.
   return false;
 }
}
Steinberg::tresult Controller::sendEditMessage(Steinberg::Vst::IMessage* message) {
 const auto result=sendMessage(message);
 if(result==Steinberg::kResultOk)dirtyPending_=true;
 return result;
}
namespace {
constexpr Steinberg::uint32 kZoomControllerState=0x314D5A44u; // DZM1 100/150
constexpr Steinberg::uint32 kZoomControllerStateV2=0x324D5A44u; // DZM2 100/120/130/150
constexpr Steinberg::uint32 kOldState=0x31524D44u;
constexpr Steinberg::uint32 kNewState=0x32524D44u;
constexpr auto kRowsState=kStateMagicV3;
}
// DZM3 extends controller-only zoom state with optional sample-folder origins.
// Embedded PCM remains in the Processor kit and never depends on these paths.
Steinberg::tresult PLUGIN_API Controller::getState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 constexpr Steinberg::uint32 kPathState=0x334D5A44u; // DZM3
 const Steinberg::uint8 zoom=preferredZoom_>1.45?3:
       preferredZoom_>1.25?2:preferredZoom_>1.05?1:0;
 if(!stream.writeInt32u(kPathState)||!stream.writeInt8u(zoom))
   return Steinberg::kResultFalse;
 for(const auto& path:samplePaths_) {
   if(path.size()>4096)return Steinberg::kResultFalse;
   if(!stream.writeInt16u(static_cast<Steinberg::uint16>(path.size())))
     return Steinberg::kResultFalse;
   if(!path.empty()&&stream.writeRaw(path.data(),static_cast<Steinberg::TSize>(path.size()))!=
                      static_cast<Steinberg::TSize>(path.size()))
     return Steinberg::kResultFalse;
 }
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Controller::setState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 constexpr Steinberg::uint32 kPathState=0x334D5A44u; // DZM3
 Steinberg::uint32 magic=0;
 Steinberg::uint8 zoom=0;
 if(!stream.readInt32u(magic)||!stream.readInt8u(zoom))
   return Steinberg::kResultFalse;
 if((magic==kZoomControllerState&&zoom>1)||
    ((magic==kZoomControllerStateV2||magic==kPathState)&&zoom>3)||
    (magic!=kZoomControllerState&&magic!=kZoomControllerStateV2&&magic!=kPathState))
   return Steinberg::kResultFalse;
 std::array<std::string,kRows> candidatePaths{};
 if(magic==kPathState) {
   try {
     for(auto& path:candidatePaths) {
       Steinberg::uint16 length=0;
       if(!stream.readInt16u(length)||length>4096)return Steinberg::kResultFalse;
       if(length) {
         path.resize(length);
         if(stream.readRaw(path.data(),length)!=length||
            path.find('\0')!=std::string::npos)
           return Steinberg::kResultFalse;
       }
     }
   }catch(const std::bad_alloc&){return Steinberg::kOutOfMemory;}
 }
 const double requested=magic==kZoomControllerState?(zoom?1.5:1.0):
     (zoom==0?1.0:zoom==1?1.2:zoom==2?1.3:1.5);
 std::size_t needed=0;
 DrumatorView* target=nullptr;
 for(auto* view:views_)if(view&&!view->zoomMatches(requested)){++needed;target=view;}
 if(!zoomRecallIsAtomic(needed))return Steinberg::kResultFalse;
 if(target&&!target->applyZoomFromState(requested))return Steinberg::kResultFalse;
 preferredZoom_=requested;
 // Both VST3 state orderings are legal. Controller-first allows exactly
 // ONE forthcoming Component restore to retain DZM3 paths. Component-first
 // has ALREADY completed that pairing: newly populated paths must never
 // arm an unrelated future Component-only restore.
 // A prior unpaired Component-only recall may have invalidated an
 // existing origin path. A subsequent DZM3 stream restoring that *known*
 // old origin belongs to a NEW Controller-first project pair; otherwise
 // the just-restored Component and DZM3 belong to the SAME current pair.
 // Comparing saved path strings requires no filesystem access or allocation.
 bool returningToKnownOrigin=false;
 for(std::size_t row=0;row<kRows;++row)
   if(!candidatePaths[row].empty()&&
      candidatePaths[row]==invalidatedSampleOrigins_[row])
     returningToKnownOrigin=true;
 const bool pairedComponentFirst=componentStatePendingController_&&
                                 !returningToKnownOrigin;
 pathsRestoredBeforeComponent_=!pairedComponentFirst;
 componentStatePendingController_=false;
 samplePaths_.swap(candidatePaths);
 for(auto& stale:invalidatedSampleOrigins_)stale.clear();
 if(pairedComponentFirst) {
   // The component just provided each row's PCM identity. The paired DZM3
   // now provides its optional path; associate them so an unchanged
   // Component-only recall can safely retain the path by identical bytes.
   for(std::size_t row=0;row<kRows;++row) {
     if(!samplePresent_[row])samplePaths_[row].clear();
     originIdentityValid_[row]=!samplePaths_[row].empty();
   }
 }
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context) {
 const auto result=EditController::initialize(context);
 if(result!=Steinberg::kResultOk)return result;
 auto* patterns=new LegacyPatternParameter(STR16("Pattern"),kPatternParam);
 patterns->appendString(STR16("A"));patterns->appendString(STR16("B"));
 patterns->appendString(STR16("C"));patterns->appendString(STR16("D"));
 patterns->appendString(STR16("E"));patterns->appendString(STR16("F"));
 patterns->appendString(STR16("G"));patterns->appendString(STR16("H"));
 parameters.addParameter(patterns);
 // Factory A alone receives the kick; B-H remain entirely blank.
 for(std::size_t step=0;step<kSteps;step+=4)
   grid_[step]=(step%8==0)?127:90;
 for(auto& name:labels_)name="EMPTY - LOAD WAV";
 gain_.fill(100);pan_.fill(100);pitch_.fill(24);choke_.fill(0);
 output_.fill(0);
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Controller::setComponentState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 Steinberg::uint32 magic=0,next=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(next))
    return Steinberg::kResultFalse;
 if(magic==kKitMagic||magic==kKitMagicV15||magic==kKitMagicV14||magic==kKitMagicV13||magic==kKitMagicV12||magic==kKitMagicV11||magic==kKitMagicV10||magic==kKitMagicV9||magic==kKitMagicV8||magic==kKitMagicV7||magic==kKitMagicV6||magic==kKitMagicV5||magic==kKitMagicV4) {
    if(next<2+kCells+kRows*6+4||next>kMaxKitBytes-8)
       return Steinberg::kResultFalse;
    KitState candidate;
    PreparedKitNames<kRows> stagedNames;
    try {
       std::vector<std::uint8_t> encoded(8+static_cast<std::size_t>(next));
       for(int i=0;i<4;++i) {
           encoded[i]=static_cast<std::uint8_t>(magic>>(8*i));
           encoded[4+i]=static_cast<std::uint8_t>(next>>(8*i));
       }
       if(stream.readRaw(encoded.data()+8,next)!=static_cast<Steinberg::TSize>(next)||
          !KitCodec::decode(encoded.data(),encoded.size(),candidate))
          return Steinberg::kResultFalse;
       // Every potentially allocating string copy happens BEFORE changing
       // edit tokens, controller grid, undo history or active kit metadata.
       stagedNames.prepare(candidate.displayNames,candidate.names);
    }catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}

    // Nothrow commit phase; a failed prepare leaves the entire old kit intact.
    invalidateTransientEditsForRecall();
    grid_=candidate.grid;
    patternLengths_=candidate.patternLengths;
    stagedNames.commit(customNames_,labels_);
    for(std::size_t row=0;row<kRows;++row)
      samplePresent_[row]=!candidate.wav[row].empty();
    undo_.reset();
    clipboardValid_=false;
    rowClipboardValid_=false;
    // Component-only recall MUST NOT inherit a stale same-named sample origin.
    // Paired Controller-before-Component recall is explicitly allowed once;
    // otherwise only an identical previously observed asset can retain origin.
    for(std::size_t row=0;row<kRows;++row) {
      auto& path=samplePaths_[row];
      const auto last=path.find_last_of("/\\\\");
      const auto begin=last==std::string::npos?0:last+1;
      const auto identity=wavIdentity(candidate.wav[row]);
      const bool sameName=!candidate.names[row].empty()&&
          path.compare(begin,std::string::npos,candidate.names[row])==0;
      if(!sameName||(!pathsRestoredBeforeComponent_&&
                     (!originIdentityValid_[row]||originIdentity_[row]!=identity))) {
        // Keep the old pathname as an allocation-free, controller-owned
        // witness for a subsequent explicit DZM3 restoring that origin.
        // Never let it authorize an unrelated Component-only recall.
        if(!path.empty())invalidatedSampleOrigins_[row].swap(path);
        path.clear();
      }
      originIdentity_[row]=identity;
      originIdentityValid_[row]=!path.empty();
    }
    if(pathsRestoredBeforeComponent_) {
      pathsRestoredBeforeComponent_=false;
      componentStatePendingController_=false;
    }else {
      componentStatePendingController_=true;
    }
    visibleRows_=candidate.visible;
    swingPercent_=candidate.swing;
    humanizePercent_=candidate.humanize;
    masterVolume_=candidate.masterVolume;
    randomVelocity_=candidate.randomVelocity;
    songEnabled_=candidate.songEnabled!=0;
    songChain_=candidate.chain;
    songStartBar_=candidate.songStartBar;
    for(std::size_t row=0;row<kRows;++row) {
       muted_[row]=(candidate.muted&(1u<<row))!=0;
       solo_[row]=(RowVisibility::visibleSolo(candidate.solo,candidate.visible)&
                   (1u<<row))!=0;
       gain_[row]=candidate.gain[row];pan_[row]=candidate.pan[row];
       pitch_[row]=candidate.pitch[row];choke_[row]=candidate.choke[row];output_[row]=candidate.output[row];
    }
    // Native focus operations may allocate. State is already committed:
    // failure to refresh an editor must not be reported as rejected kit state.
    // Update the registered parameter independently from native editor focus.
    // A failed HWND teardown must not leave the selected parameter stale.
    const auto restoredPattern=hostPatternPending_&&processorConfirmsPattern(patternFromNormalized(hostPatternValue_))
        ? hostPatternValue_ : static_cast<double>(candidate.selected)/7.0;
    hostPatternPending_=false;
    Steinberg::tresult selectedResult=Steinberg::kResultFalse;
    try {
      selectedResult=EditController::setParamNormalized(kPatternParam,restoredPattern);
    }catch(const std::bad_alloc&) {
      // Recall state has already committed; UI parameter refresh is best-effort.
    }
    for(auto* view:views_)if(view) {
       try {view->finishTransientEditsAfterRecall();}
       catch(const std::bad_alloc&) { /* old edit token already invalid */ }
    }
    if(selectedResult==Steinberg::kResultOk) {
      try {notifyViews(true);}
      catch(const std::bad_alloc&) { /* state already committed */ }
    }
    return Steinberg::kResultOk;
 }
 if(next>=kPatterns)return Steinberg::kResultFalse;
 // Stage the 16-byte default label (possibly beyond SSO) before any edit
 // invalidation, even for the legacy project-state compatibility paths.
 PreparedKitNames<kRows> legacyNames;
 try {
   const std::array<std::string,kRows> empty{};
   legacyNames.prepare(empty,empty);
 }catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
 std::uint8_t candidateRows=static_cast<std::uint8_t>(kRows);
 if(magic==kRowsState) {
   if(!stream.readInt8u(candidateRows)||!RowVisibility::valid(candidateRows))
      return Steinberg::kResultFalse;
 }
 if(magic==kNewState||magic==kRowsState) {
   std::array<std::uint8_t,kCells> candidate{};
   for(auto& step:candidate)
       if(!stream.readInt8u(step)||step>127)return Steinberg::kResultFalse;
   invalidateTransientEditsForRecall();
   grid_=candidate;
 }else if(magic==kOldState) {
   // DMR1 contains no grid; restore the identical factory pattern used by
   // the processor instead of retaining edits from the preceding project.
   invalidateTransientEditsForRecall();
   grid_.fill(0);
   for(std::size_t p=0;p<kPatterns;++p)
     for(std::size_t step=0;step<kSteps;step+=4)
       grid_[(p*kRows)*kSteps+step]=(step%8==0)?127:90;
 }else return Steinberg::kResultFalse;
 visibleRows_=candidateRows;
 patternLengths_.fill(32);
 legacyNames.commit(customNames_,labels_);
 undo_.reset();
 // Successful legacy state already invalidated all pending edits.
 clipboardValid_=false;
 rowClipboardValid_=false;
 for(auto& path:samplePaths_)path.clear();
 samplePresent_.fill(false);
 muted_.fill(false);solo_.fill(false);swingPercent_=0;humanizePercent_=0;
 randomVelocity_.fill(0);masterVolume_=100;
 songEnabled_=false;songChain_=SongChain{};songStartBar_=1;
 gain_.fill(100);pan_.fill(100);pitch_.fill(24);choke_.fill(0);
 for(std::size_t row=0;row<kRows;++row)output_[row]=static_cast<std::uint8_t>(row);
 const auto restoredPattern=hostPatternPending_&&processorConfirmsPattern(patternFromNormalized(hostPatternValue_))
     ? hostPatternValue_ : static_cast<double>(next)/7.0;
 hostPatternPending_=false;
 Steinberg::tresult selectedResult=Steinberg::kResultFalse;
 try {
   selectedResult=EditController::setParamNormalized(kPatternParam,restoredPattern);
 }catch(const std::bad_alloc&) {
   // Recall state has already committed; UI parameter refresh is best-effort.
 }
 for(auto* view:views_)if(view) {
   try {view->finishTransientEditsAfterRecall();}
   catch(const std::bad_alloc&) { /* legacy model already committed */ }
 }
 if(selectedResult==Steinberg::kResultOk) {
   try {notifyViews(true);}
   catch(const std::bad_alloc&) { /* legacy model already committed */ }
 }
 return Steinberg::kResultOk;
}
Steinberg::IPlugView* PLUGIN_API Controller::createView(Steinberg::FIDString name) {
 if(name&&std::strcmp(name,Steinberg::Vst::ViewType::kEditor)==0) {
    auto* editor=new DrumatorEditor(this,"DrumatorView","Drumator.uidesc");
    // Disable inherited VSTGUI Zoom context menu: it bypasses checked
    // host resize and persistent zoom. Visible 100/150 controls remain.
    editor->setAllowedZoomFactors({});
    editor->setZoomChecked(preferredZoom_);
    return editor;
 }
 return nullptr;
}
VSTGUI::CView* Controller::createCustomView(VSTGUI::UTF8StringPtr name,
          const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
          VSTGUI::VST3Editor* editor) {
 if(!name||std::strcmp(name,"DrumatorStepGrid")!=0)return nullptr;
 return new DrumatorView(VSTGUI::CRect(0,0,1280,300),*this,*editor);
}
std::uint8_t Controller::velocity(std::size_t p,std::size_t r,std::size_t s) const noexcept {
 if(p>=kPatterns||r>=kRows||s>=kSteps)return 0;
 return grid_[(p*kRows+r)*kSteps+s];
}
std::size_t Controller::currentPattern() noexcept {
 const double value=getParamNormalized(kPatternParam);
 if(!std::isfinite(value))return 0;
 return static_cast<std::size_t>(patternFromNormalized(value));
}
const std::string& Controller::label(std::size_t row) const noexcept {
 static const std::string empty;
 return row<kRows?(!customNames_[row].empty()?customNames_[row]:labels_[row]):empty;
}

bool Controller::renameRow(std::size_t row,const std::string& name) {
 if(row>=kRows||!validRowLabel(name))return false;
 if(customNames_[row]==name)return true;
 // Allocate BEFORE asking the processor to commit. Updating the controller
 // after success must be noexcept, or an OOM leaves both sides inconsistent.
 std::string prepared;
 try {prepared=name;}catch(const std::bad_alloc&) {return false;}
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RenameRow");
 auto* attrs=msg->getAttributes();
 if(!attrs||attrs->setInt("Row",static_cast<Steinberg::int64>(row))!=Steinberg::kResultOk||
    attrs->setBinary("DisplayName",name.data(),static_cast<Steinberg::uint32>(name.size()))!=Steinberg::kResultOk){
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){customNames_[row].swap(prepared);notifyViews(false);}
 return ok;
}
bool Controller::configureSong(bool enabled,std::size_t count,const SongChain& chain,std::uint16_t startBar) {
 if(startBar==0)startBar=songStartBar_;
 if(startBar<1||startBar>9999)return false;
 if(count==0||count>SongChain::kCapacity||chain.count()!=count)return false;
 std::array<std::uint8_t,2*SongChain::kCapacity> bytes{};
 for(std::size_t i=0;i<count;++i){
   const auto entry=chain.entry(i);
   if(entry.pattern>SongChain::kRest||!SongChain::validLength(entry.steps))return false;
   bytes[2*i]=entry.pattern;bytes[2*i+1]=entry.steps;
 }
 auto* msg=allocateMessage();
 if(!msg)return false;
 msg->setMessageID("Drumator.SongConfig");
 auto* a=msg->getAttributes();
 if(!a||a->setInt("Enabled",enabled?1:0)!=Steinberg::kResultOk||
    a->setInt("Count",static_cast<Steinberg::int64>(count))!=Steinberg::kResultOk||
    a->setInt("StartBar",startBar)!=Steinberg::kResultOk||
    a->setBinary("Entries",bytes.data(),static_cast<Steinberg::uint32>(count*2))!=Steinberg::kResultOk){
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){songEnabled_=enabled;songChain_=chain;songStartBar_=startBar;notifyViews(false);}
 return ok;
}
bool Controller::setSongStartBar(unsigned bar){
 return bar>=1&&bar<=9999&&configureSong(songEnabled_,songChain_.count(),songChain_,static_cast<std::uint16_t>(bar));
}
bool Controller::toggleSongMode(){return configureSong(!songEnabled_,songChain_.count(),songChain_);}
bool Controller::adjustSongCount(int delta){
 const auto old=static_cast<int>(songChain_.count());
 // Clamp in a wider signed domain: int deltas may be INT_MIN or INT_MAX.
 const auto next=std::clamp(static_cast<std::int64_t>(old)+static_cast<std::int64_t>(delta),
                            std::int64_t{1},static_cast<std::int64_t>(SongChain::kCapacity));
 SongChain candidate=songChain_;
 if(!candidate.setCount(static_cast<std::size_t>(next)))return false;
 return configureSong(songEnabled_,candidate.count(),candidate);
}
bool Controller::setSongEntry(std::size_t slot,std::uint8_t pattern,std::uint8_t steps){
 if(slot>=songChain_.count()||pattern>SongChain::kRest||!SongChain::validLength(steps))return false;
 SongChain candidate=songChain_;
 if(!candidate.configure(slot,pattern,steps))return false;
 return configureSong(songEnabled_,candidate.count(),candidate);
}
bool Controller::editSongEntry(std::size_t slot,int patternDelta,int lengthDelta){
 if(slot>=songChain_.count())return false;
 SongChain candidate=songChain_;
 const auto old=candidate.entry(slot);
 // Reduce first: adding a raw int delta (e.g. INT_MAX/INT_MIN) could
 // overflow before modulo or produce a negative array index.
 const auto wrap=[](std::int64_t index,std::int64_t delta,std::int64_t count) noexcept {
   const auto remainder=delta%count;
   const auto sum=(index+remainder)%count;
   return static_cast<std::size_t>(sum<0?sum+count:sum);
 };
 const auto pattern=static_cast<std::uint8_t>(
     wrap(old.pattern==SongChain::kRest?0:old.pattern,patternDelta,static_cast<std::int64_t>(kPatterns)));
 const std::array<std::uint8_t,3> lengths{8,16,32};
 const std::int64_t index=old.steps==8?0:(old.steps==16?1:2);
 const auto length=lengths[wrap(index,lengthDelta,3)];
 if(!candidate.configure(slot,pattern,length))return false;
 return configureSong(songEnabled_,candidate.count(),candidate);
}

bool Controller::togglePatternLength(std::size_t pattern) {
 if(pattern>=kPatterns)return false;
 const auto next=static_cast<std::uint8_t>(patternLengths_[pattern]==16?32:16);
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.PatternLength");
 auto* a=msg->getAttributes();
 if(!a||a->setInt("Pattern",static_cast<Steinberg::int64>(pattern))!=Steinberg::kResultOk||
    a->setInt("Steps",next)!=Steinberg::kResultOk) {
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){patternLengths_[pattern]=next;notifyViews(false);}
 return ok;
}
bool Controller::changePattern(std::size_t pattern) {
 if(pattern>=kPatterns)return false;
 const auto value=static_cast<double>(pattern)/7.0;
 beginEdit(kPatternParam);
 const auto done=performEdit(kPatternParam,value);
 endEdit(kPatternParam);
 if(done!=Steinberg::kResultOk)return false;
 setParamNormalized(kPatternParam,value);
 return true;
}
bool Controller::replacePattern(std::size_t p,const PatternSteps& data,bool remember,
                                std::uint8_t requestedLength) {
 if(p>=kPatterns||(requestedLength!=0&&requestedLength!=16&&requestedLength!=32))
   return false;
 if(strokePattern_<kPatterns)endVelocityStroke(true,strokeOwner_);
 const auto oldLength=patternLengths_[p];
 const auto nextLength=requestedLength==0?oldLength:requestedLength;
 PatternSteps old{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,old.size(),old.begin());
 if(old==data&&oldLength==nextLength)return true;
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.PatternReplace");
 auto* a=msg->getAttributes();
 if(!a||a->setInt("Pattern",static_cast<Steinberg::int64>(p))!=Steinberg::kResultOk||
    a->setBinary("Steps",data.data(),static_cast<Steinberg::uint32>(data.size()))!=Steinberg::kResultOk||
    (requestedLength!=0&&a->setInt("Length",nextLength)!=Steinberg::kResultOk)){
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){
   if(remember)undo_.capture(p,old,data,oldLength,nextLength);
   std::copy(data.begin(),data.end(),grid_.begin()+p*kRows*kSteps);
   patternLengths_[p]=nextLength;
   notifyViews(false);
 }
 return ok;
}
bool Controller::copyCurrentPattern() {
 const auto p=currentPattern();
 std::copy_n(grid_.begin()+p*kRows*kSteps,clipboard_.size(),clipboard_.begin());
 clipboardLength_=patternLengths_[p];
 clipboardValid_=true;return true;
}
bool Controller::pasteCurrentPattern(){
 return clipboardValid_&&replacePattern(currentPattern(),clipboard_,true,clipboardLength_);
}
bool Controller::copyRow(std::size_t row) noexcept {
 if(row>=visibleRows_)return false;
 const auto p=currentPattern();
 std::copy_n(grid_.begin()+(p*kRows+row)*kSteps,kSteps,rowClipboard_.begin());
 rowClipboardValid_=true;
 return true;
}
bool Controller::pasteRow(std::size_t row) {
 if(row>=visibleRows_||!rowClipboardValid_)return false;
 const auto p=currentPattern();
 PatternSteps target{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,target.size(),target.begin());
 std::copy(rowClipboard_.begin(),rowClipboard_.end(),target.begin()+row*kSteps);
 // Full transactional replacement records a single target-pattern Undo.
 // Its musical length, other rows, and all sound/routing metadata are kept.
 return replacePattern(p,target);
}
bool Controller::canDuplicateCurrentPattern() noexcept {
 const auto source=currentPattern();
 PatternSteps content{};
 std::copy_n(grid_.begin()+source*kRows*kSteps,content.size(),content.begin());
 if(emptyPattern(content))return false;
 return nextEmptyPattern(source,[&](std::size_t index){
   PatternSteps candidate{};
   std::copy_n(grid_.begin()+index*kRows*kSteps,candidate.size(),candidate.begin());
   return candidate;
 })<kPatterns;
}
bool Controller::duplicateCurrentPattern(bool* selectedCopy){
 if(selectedCopy)*selectedCopy=false;
 const auto source=currentPattern();
 PatternSteps data{};
 std::copy_n(grid_.begin()+source*kRows*kSteps,data.size(),data.begin());
 if(emptyPattern(data))return false; // an empty pattern cannot be duplicated meaningfully
 const auto destination=nextEmptyPattern(source,[&](std::size_t candidate){
   PatternSteps data{};
   std::copy_n(grid_.begin()+candidate*kRows*kSteps,data.size(),data.begin());
   return data;
 });
 if(destination>=kPatterns)return false; // never replace an occupied beat
 if(!replacePattern(destination,data,true,patternLengths_[source]))return false;
 const bool switched=changePattern(destination);
 if(selectedCopy)*selectedCopy=switched;
 return true; // the copy exists even if host refuses selecting it
}
bool Controller::clearCurrentPattern(){return replacePattern(currentPattern(),{});}
bool Controller::undoCurrentPattern(){
 const auto p=currentPattern();
 if(strokePattern_<kPatterns)endVelocityStroke(true,strokeOwner_);
 const auto* old=undo_.snapshot(p);
 if(!old)return false;
 const PatternSteps immutable=*old;
 const auto length=undo_.snapshotLength(p);
 if(!replacePattern(p,immutable,false,length))return false;
 undo_.consume(p);
 return true;
}
bool Controller::randomizeRowVelocity(std::size_t row){
 if(row>=visibleRows_)return false;
 const auto p=currentPattern();
 const auto active=static_cast<std::size_t>(patternLength(p));
 if(active!=16&&active!=32)return false;
 PatternSteps data{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,data.size(),data.begin());
 auto candidate=data;
 const auto variation=velocityVariation_+1u;
 bool any=false;
 for(std::size_t step=0;step<active;++step){
   const auto index=row*kSteps+step;
   if(data[index]==0)continue; // never invent new notes
   any=true;
   std::uint32_t hash=static_cast<std::uint32_t>((row+1u)*0x9e3779b9u)^
       static_cast<std::uint32_t>((step+1u)*0x85ebca6bu)^(variation*0xc2b2ae35u);
   hash^=hash>>16;hash*=0x7feb352du;hash^=hash>>15;
   hash*=0x846ca68bu;hash^=hash>>16;
   const int offset=static_cast<int>(hash%49u)-24;
   const int source=static_cast<int>(data[index]);
   int next=std::clamp(source+offset,1,127);
   // A press must visibly change every existing note, including saturated
   // source values 1 and 127, rather than looking like a no-op.
   if(next==source)next=source==127?126:source==1?2:
       source+((hash&1u)?1:-1);
   candidate[index]=static_cast<std::uint8_t>(next);
 }
 if(!any)return false; // no content and no undo entry
 if(!replacePattern(p,candidate))return false;
 velocityVariation_=variation;
 return true;
}
bool Controller::varyPatternVelocity(){
 const auto p=currentPattern();
 const auto active=static_cast<std::size_t>(patternLength(p));
 if(active!=16&&active!=32)return false;
 PatternSteps data{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,data.size(),data.begin());
 // Deterministic per edit: change ONLY currently active steps. A 1-bar
 // pattern must preserve bars 2 data for a subsequent 2-bar reactivation.
 const auto variation=velocityVariation_+1u;
 for(std::size_t row=0;row<kRows;++row)
  for(std::size_t step=0;step<active;++step){
   const auto index=row*kSteps+step;
   if(data[index]==0)continue;
   std::uint32_t hash=static_cast<std::uint32_t>((row+1u)*0x9e3779b9u)^
       static_cast<std::uint32_t>((step+1u)*0x85ebca6bu)^(variation*0xc2b2ae35u);
   hash^=hash>>16;hash*=0x7feb352du;hash^=hash>>15;hash*=0x846ca68bu;hash^=hash>>16;
   const int offset=static_cast<int>(hash%49u)-24;
   const int value=std::clamp(static_cast<int>(data[index])+offset,1,127);
   data[index]=static_cast<std::uint8_t>(value);
  }
 if(!replacePattern(p,data))return false;
 velocityVariation_=variation;
 return true;
}
bool Controller::resetPatternVelocity(){
 const auto p=currentPattern();
 const auto active=static_cast<std::size_t>(patternLength(p));
 if(active!=16&&active!=32)return false;
 PatternSteps data{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,data.size(),data.begin());
 // Do not reset invisible steps 17-32 while editing a 1-bar pattern.
 for(std::size_t row=0;row<kRows;++row)
   for(std::size_t step=0;step<active;++step) {
     auto& value=data[row*kSteps+step];
     if(value!=0)value=100;
   }
 return replacePattern(p,data);
}
bool Controller::fillRow(std::size_t row,RowFill mode){
 if(row>=visibleRows_)return false;
 const auto p=currentPattern();
 const auto active=static_cast<std::size_t>(patternLength(p));
 if(active!=16&&active!=32)return false;
 PatternSteps data{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,data.size(),data.begin());
 return replacePattern(p,fillInstrument(data,row,mode,active));
}
bool Controller::canHideLast() const noexcept {
 if(visibleRows_<=1||visibleRows_>kRows)return false;
 const auto row=static_cast<std::size_t>(visibleRows_-1);
 if(labels_[row]!="EMPTY - LOAD WAV"||!customNames_[row].empty()||solo_[row])return false;
 struct GridView {
  const Controller& ctrl;
  std::uint8_t get(std::size_t p,std::size_t r,std::size_t s) const noexcept {
   return ctrl.velocity(p,r,s);
  }
 };
 return RowVisibility::canHideLast(visibleRows_,GridView{*this},false);
}
bool Controller::addRow() {
 if(visibleRows_>=kRows)return false;
 auto* message=allocateMessage();
 if(!message)return false;
 message->setMessageID("Drumator.VisibleRows");
 auto* attrs=message->getAttributes();
 if(!attrs){message->release();return false;}
 attrs->setInt("Count",static_cast<Steinberg::int64>(visibleRows_+1));
 const auto good=sendEditMessage(message)==Steinberg::kResultOk;
 message->release();
 if(good){++visibleRows_;notifyViews(false);}
 return good;
}
bool Controller::restoreVisibleRows(std::size_t previous) {
 if(previous<1||previous>kRows)return false;
 if(previous==visibleRows_)return true;
 auto* message=allocateMessage();
 if(!message)return false;
 message->setMessageID("Drumator.VisibleRows");
 auto* attrs=message->getAttributes();
 if(!attrs){message->release();return false;}
 attrs->setInt("Count",static_cast<Steinberg::int64>(previous));
 const bool good=sendEditMessage(message)==Steinberg::kResultOk;
 message->release();
 if(good){visibleRows_=static_cast<std::uint8_t>(previous);notifyViews(false);}
 return good;
}
bool Controller::hideLastEmptyRow() {
 if(!canHideLast())return false;
 auto* message=allocateMessage();
 if(!message)return false;
 message->setMessageID("Drumator.VisibleRows");
 auto* attrs=message->getAttributes();
 if(!attrs){message->release();return false;}
 attrs->setInt("Count",static_cast<Steinberg::int64>(visibleRows_-1));
 const auto good=sendEditMessage(message)==Steinberg::kResultOk;
 message->release();
 if(good){--visibleRows_;notifyViews(false);}
 return good;
}
void Controller::beginVelocityStroke(std::size_t p,const void* owner) {
 if(p>=kPatterns)return;
 if(strokePattern_<kPatterns)endVelocityStroke(true,strokeOwner_);
 strokeOwner_=owner;
 strokePattern_=p;
 strokeGeneration_=editGeneration_.current();
 std::copy_n(grid_.begin()+p*kRows*kSteps,strokeBefore_.size(),strokeBefore_.begin());
}
void Controller::endVelocityStroke(bool commit,const void* owner) {
 if(strokePattern_>=kPatterns||owner!=strokeOwner_)return;
 const auto p=strokePattern_;
 strokePattern_=kPatterns;
 strokeOwner_=nullptr;
 if(!editGeneration_.matches(strokeGeneration_))return;
 strokeGeneration_=0;
 PatternSteps after{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,after.size(),after.begin());
 if(!commit&&after!=strokeBefore_&&replacePattern(p,strokeBefore_,false))
   return;
 undo_.capture(p,strokeBefore_,after);
}
void Controller::rememberUndoStep(std::size_t p,std::size_t r,std::size_t s,std::uint8_t next) {
 PatternSteps old{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,old.size(),old.begin());
 auto changed=old;changed[r*kSteps+s]=next;
 undo_.capture(p,old,changed);
}
bool Controller::setStepVelocity(std::size_t p,std::size_t r,std::size_t s,std::uint8_t next,const void* owner){
 if(p>=kPatterns||r>=kRows||s>=kSteps||next>127)return false;
 // Peer edits must commit the previous owner's gesture before mutation.
 // Late Cancel from that owner is then inert; no stale whole-grid restore.
 if(strokePattern_<kPatterns&&strokeOwner_!=owner)
   endVelocityStroke(true,strokeOwner_);
 if(velocity(p,r,s)==next)return true;
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.StepEdit");
 auto* a=msg->getAttributes();
 if(!a||a->setInt("Pattern",static_cast<Steinberg::int64>(p))!=Steinberg::kResultOk||
    a->setInt("Row",static_cast<Steinberg::int64>(r))!=Steinberg::kResultOk||
    a->setInt("Step",static_cast<Steinberg::int64>(s))!=Steinberg::kResultOk||
    a->setInt("Velocity",static_cast<Steinberg::int64>(next))!=Steinberg::kResultOk) {
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok) {
   if(strokePattern_!=p)rememberUndoStep(p,r,s,next);
   grid_[(p*kRows+r)*kSteps+s]=next;
   notifyViews(false);
 }
 return ok;
}
bool Controller::cycleStepVelocity(std::size_t p,std::size_t r,std::size_t s) {
 if(p>=kPatterns||r>=kRows||s>=kSteps)return false;
 return setStepVelocity(p,r,s,drumator::nextStepVelocity(velocity(p,r,s)));
}
bool Controller::adjustRandomVelocity(std::size_t row,int delta) {
 if(row>=kRows)return false;
 const auto value=std::clamp(static_cast<std::int64_t>(randomVelocity_[row])+delta,std::int64_t{0},std::int64_t{100});
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowRandomVelocity");
 auto* attrs=msg->getAttributes();
 if(!attrs||attrs->setInt("Row",static_cast<Steinberg::int64>(row))!=Steinberg::kResultOk||
    attrs->setInt("Percent",value)!=Steinberg::kResultOk) {
   msg->release();return false;
 }
 const auto ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){randomVelocity_[row]=static_cast<std::uint8_t>(value);notifyViews(false);}
 return ok;
}
bool Controller::adjustMasterVolume(int delta) {
 const auto value=std::clamp(static_cast<std::int64_t>(masterVolume_)+delta,std::int64_t{0},std::int64_t{200});
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.MasterVolume");
 auto* attrs=msg->getAttributes();
 if(!attrs||attrs->setInt("Percent",value)!=Steinberg::kResultOk){
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){masterVolume_=static_cast<std::uint8_t>(value);notifyViews(false);}
 return ok;
}
bool Controller::adjustHumanize(int delta) {
 const auto next=std::clamp(static_cast<std::int64_t>(humanizePercent_)+delta,std::int64_t{0},std::int64_t{100});
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.Humanize");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Percent",next);
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();if(ok){humanizePercent_=static_cast<std::uint8_t>(next);notifyViews(false);}
 return ok;
}
bool Controller::adjustSwing(int delta) {
 const auto next=std::clamp(static_cast<std::int64_t>(swingPercent_)+delta,std::int64_t{0},std::int64_t{50});
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.Swing");
 auto* attr=msg->getAttributes();if(!attr){msg->release();return false;}
 attr->setInt("Percent",next);
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){swingPercent_=static_cast<std::uint8_t>(next);notifyViews(false);}
 return ok;
}

bool Controller::sampleEditorSnapshot(std::size_t row,SampleEdit& edit,
                                      std::uint32_t& frames,
                                      std::array<WaveformRange,256>& envelope,
                                      std::uint32_t first,std::uint32_t last,
                                      std::uint32_t* revision,
                                      std::array<WaveformRange,256>* rightEnvelope,
                                      bool* stereo) {
 if(row>=kRows)return false;
 Steinberg::Vst::IMessage* msg=nullptr;
 try {msg=allocateMessage();}catch(const std::bad_alloc&){return false;}
 if(!msg)return false;
 struct MessageOwner {decltype(msg) value;~MessageOwner(){if(value)value->release();}} owner{msg};
 try {
   msg->setMessageID("Drumator.SampleEditorQuery");
   auto* a=msg->getAttributes();
   if(!a||a->setInt("Row",static_cast<Steinberg::int64>(row))!=Steinberg::kResultOk)
     return false;
   // A metadata-only roundtrip detects another View's source replacement.
   if(editorWaveRow_==row&&
      a->setInt("KnownRevision",editorWaveRevision_)!=Steinberg::kResultOk)
     return false;
   if(sendMessage(msg)!=Steinberg::kResultOk)return false;
   Steinberg::int64 start=0,end=0,fadeIn=0,fadeOut=0,gainBits=0,version=-1;
   if(a->getInt("Start",start)!=Steinberg::kResultOk||
      a->getInt("End",end)!=Steinberg::kResultOk||
      a->getInt("FadeIn",fadeIn)!=Steinberg::kResultOk||
      a->getInt("FadeOut",fadeOut)!=Steinberg::kResultOk||
      a->getInt("GainBits",gainBits)!=Steinberg::kResultOk||
      a->getInt("Revision",version)!=Steinberg::kResultOk||
      version<0||version>UINT32_MAX||
      start<0||end<0||fadeIn<0||fadeOut<0||
      start>UINT32_MAX||end>UINT32_MAX||fadeIn>UINT32_MAX||
      fadeOut>UINT32_MAX||gainBits<0||gainBits>UINT32_MAX)
     return false;
   const auto rev=static_cast<std::uint32_t>(version);
   const bool hit=editorWaveRow_==row&&editorWaveRevision_==rev;
   DecodedWav fresh;
   WaveformExtremaIndex stagedLeft,stagedRight;
   const DecodedWav* wave=&editorWaveCache_;
   if(!hit) {
     const void* bytes=nullptr;Steinberg::uint32 length=0;
     if(a->getBinary("Wave",bytes,length)!=Steinberg::kResultOk||
        !bytes||WavPcm16Decoder::decode(static_cast<const std::uint8_t*>(bytes),
                                        length,fresh)!=WavError::Ok||
        fresh.left.size()>UINT32_MAX)
       return false;
     // Full source scan is done once per accepted PCM revision, never in audio.
     if(!stagedLeft.build(fresh.left.data(),fresh.left.size())||
        (fresh.channels==2&&
         !stagedRight.build(fresh.right.data(),fresh.right.size())))
       return false;
     wave=&fresh;
   }
   const auto count=static_cast<std::uint32_t>(wave->left.size());
   if(count==0||(wave->channels!=1&&wave->channels!=2))return false;
   SampleEdit next;
   next.start=static_cast<std::uint32_t>(start);
   next.end=static_cast<std::uint32_t>(end);
   next.fadeIn=static_cast<std::uint32_t>(fadeIn);
   next.fadeOut=static_cast<std::uint32_t>(fadeOut);
   const auto bits=static_cast<std::uint32_t>(gainBits);
   std::memcpy(&next.gain,&bits,sizeof(bits));
   if(next.end==0)next=SampleEdit::reset(count);
   if(!next.valid(count))return false;
   if(last==0)last=count;
   if(first>=last||last>count)return false;
   const auto& indexedLeft=hit?editorWaveIndexLeft_:stagedLeft;
   const auto& indexedRight=hit?editorWaveIndexRight_:stagedRight;
   // Preserve the original half-open bin boundaries and min/max tie order.
   for(std::size_t i=0;i<envelope.size();++i) {
     const auto binFirst=static_cast<std::size_t>(first)+
                         (static_cast<std::size_t>(last-first)*i)/envelope.size();
     const auto binLast=static_cast<std::size_t>(first)+
                        (static_cast<std::size_t>(last-first)*(i+1))/envelope.size();
     auto bounds=indexedLeft.range(wave->left.data(),wave->left.size(),
                        binFirst,std::max(binFirst+std::size_t{1},binLast));
     if(wave->channels==2) {
       const auto other=indexedRight.range(wave->right.data(),wave->right.size(),
                          binFirst,std::max(binFirst+std::size_t{1},binLast));
       if(rightEnvelope)(*rightEnvelope)[i]=other;
       else {bounds.minimum=std::min(bounds.minimum,other.minimum);
             bounds.maximum=std::max(bounds.maximum,other.maximum);}
     }
     envelope[i]=bounds;
   }
   if(!hit) {
     editorWaveIndexLeft_=std::move(stagedLeft);
     editorWaveIndexRight_=std::move(stagedRight);
     editorWaveCache_=std::move(fresh);
     editorWaveRow_=row;
     editorWaveRevision_=rev;
     wave=&editorWaveCache_;
   }
   edit=next;frames=count;
   if(stereo)*stereo=wave->channels==2;
   if(revision)*revision=rev;
   return true;
 }catch(const std::bad_alloc&){return false;}
}
bool Controller::commitSampleEdit(std::size_t row,const SampleEdit& edit,
                                  std::uint32_t revision) {
 if(row>=kRows||!edit.end)return false;
 Steinberg::Vst::IMessage* msg=nullptr;
 try {msg=allocateMessage();}catch(const std::bad_alloc&){return false;}
 if(!msg)return false;
 struct MessageOwner {decltype(msg) value;~MessageOwner(){if(value)value->release();}} owner{msg};
 try {
 msg->setMessageID("Drumator.SampleEditorCommit");
 auto* a=msg->getAttributes();
 std::uint32_t bits=0;std::memcpy(&bits,&edit.gain,sizeof(bits));
 bool ready=a&&
     a->setInt("Row",static_cast<Steinberg::int64>(row))==Steinberg::kResultOk&&
     a->setInt("Start",edit.start)==Steinberg::kResultOk&&
     a->setInt("End",edit.end)==Steinberg::kResultOk&&
     a->setInt("FadeIn",edit.fadeIn)==Steinberg::kResultOk&&
     a->setInt("FadeOut",edit.fadeOut)==Steinberg::kResultOk&&
     a->setInt("GainBits",bits)==Steinberg::kResultOk;
 if(ready&&revision!=UINT32_MAX)
   ready= a->setInt("Revision",revision)==Steinberg::kResultOk;
 const bool success=ready&&sendEditMessage(msg)==Steinberg::kResultOk;
 
 if(success){try {notifyViews(false);}catch(const std::bad_alloc&){ /* committed state remains authoritative */ }}
 return success;
 }catch(const std::bad_alloc&){return false;}
}


bool Controller::previewSampleEdit(std::size_t row,const SampleEdit& edit,
                                   std::uint32_t revision){
 if(row>=kRows||edit.end==0)return false;
 // An accepted preview must always have an allocation-free STOP path.
 // A previous rejected STOP must be delivered before accepting another START.
 if(previewStopPending_&&!stopSampleEditPreview())return false;
 if(!reservePreviewStop())return false;
 Steinberg::Vst::IMessage* msg=nullptr;
 try {msg=allocateMessage();}catch(const std::bad_alloc&){return false;}
 if(!msg)return false;
 struct MessageOwner {decltype(msg) value;~MessageOwner(){if(value)value->release();}} owner{msg};
 try {
 msg->setMessageID("Drumator.EditorPreview");
 auto* a=msg->getAttributes();std::uint32_t bits=0;
 std::memcpy(&bits,&edit.gain,sizeof(bits));
 bool ready=a&&a->setInt("Row",static_cast<Steinberg::int64>(row))==Steinberg::kResultOk&&
 a->setInt("Start",edit.start)==Steinberg::kResultOk&&
 a->setInt("End",edit.end)==Steinberg::kResultOk&&
 a->setInt("FadeIn",edit.fadeIn)==Steinberg::kResultOk&&
 a->setInt("FadeOut",edit.fadeOut)==Steinberg::kResultOk&&
 a->setInt("GainBits",bits)==Steinberg::kResultOk;
 if(ready&&revision!=UINT32_MAX)
   ready=a->setInt("Revision",revision)==Steinberg::kResultOk;
 const bool ok=ready&&sendMessage(msg)==Steinberg::kResultOk;
 return ok;
 }catch(const std::bad_alloc&){return false;}
}
bool Controller::stopSampleEditPreview(){
 // The STOP message is prepared before Preview, so retries do not allocate.
 // A bounded second attempt covers one transient host rejection while an
 // editor is being destroyed. STOP is idempotent in the processor.
 if(!reservedPreviewStop_)return true; // no editor preview accepted
 previewStopPending_=true;
 for(unsigned attempt=0;attempt<2;++attempt) {
   try {
     if(sendMessage(reservedPreviewStop_)==Steinberg::kResultOk) {
       previewStopPending_=false;
       return true;
     }
   }catch(const std::bad_alloc&) { /* retry without allocating */ }
 }
 return false;
}
bool Controller::previewSample(std::size_t row) {
 if(row>=visibleRows_)return false;
 Steinberg::Vst::IMessage* msg=nullptr;
 try {msg=allocateMessage();}
 catch(const std::bad_alloc&) {return false;}
 if(!msg)return false;
 struct MessageOwner {Steinberg::Vst::IMessage* p;~MessageOwner(){p->release();}} owner{msg};
 try {
   msg->setMessageID("Drumator.PreviewRow");
   auto* attrs=msg->getAttributes();
   if(!attrs||attrs->setInt("Row",static_cast<Steinberg::int64>(row))!=Steinberg::kResultOk)
     return false;
   // A preview must not mark the DAW project dirty.
   return sendMessage(msg)==Steinberg::kResultOk;
 }catch(const std::bad_alloc&) {return false;}
}
bool Controller::clearSample(std::size_t row) {
 if(row>=kRows)return false;
 std::string emptyLabel;
 try {emptyLabel="EMPTY - LOAD WAV";}
 catch(const std::bad_alloc&) {return false;}
 Steinberg::Vst::IMessage* msg=nullptr;
 try {msg=allocateMessage();}
 catch(const std::bad_alloc&) {return false;}
 if(!msg)return false;
 struct MessageOwner {Steinberg::Vst::IMessage* p;~MessageOwner(){p->release();}} owner{msg};
 bool accepted=false;
 try {
   msg->setMessageID("Drumator.ClearSample");
   auto* attrs=msg->getAttributes();
   if(!attrs||attrs->setInt("Row",static_cast<Steinberg::int64>(row))!=Steinberg::kResultOk)
     return false;
   accepted=sendEditMessage(msg)==Steinberg::kResultOk;
 }catch(const std::bad_alloc&) {return false;}
 if(accepted) {
   // Processor accepted: commit metadata without further allocations.
   labels_[row].swap(emptyLabel);samplePaths_[row].clear();
   samplePresent_[row]=false;
   try {notifyViews(false);}
   catch(const std::bad_alloc&) { /* committed state must not be reported as rejection */ }
 }
 return accepted;
}
bool Controller::setRowOutput(std::size_t row,unsigned requested) {
 if(row>=kRows||requested>=kRows)return false;
 if(output_[row]==requested)return true;
 const auto bus=static_cast<std::uint8_t>(requested);
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowOutput");
 auto* attrs=msg->getAttributes();
 if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 attrs->setInt("Bus",static_cast<Steinberg::int64>(bus));
 const bool accepted=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();if(accepted){output_[row]=bus;notifyViews(false);}
 return accepted;
}
bool Controller::cycleRowOutput(std::size_t row) {
 return row<kRows?setRowOutput(row,(output_[row]+1)%kRows):false;
}
bool Controller::setRowChoke(std::size_t row,unsigned requested) {
 if(row>=kRows||requested>4)return false;
 if(choke_[row]==requested)return true;
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowChoke");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 attrs->setInt("Group",static_cast<Steinberg::int64>(requested));
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){choke_[row]=static_cast<std::uint8_t>(requested);notifyViews(false);}
 return ok;
}
bool Controller::cycleRowChoke(std::size_t row,int direction) {
 if(row>=kRows||(direction!=1&&direction!=-1))return false;
 return setRowChoke(row,static_cast<unsigned>((static_cast<int>(choke_[row])+5+direction)%5));
}
bool Controller::adjustRowPitch(std::size_t row,int delta) {
 if(row>=kRows)return false;
 const int proposed=std::clamp(static_cast<std::int64_t>(pitch_[row])+delta,std::int64_t{0},std::int64_t{48});
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowPitch");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 attrs->setInt("Semitones",proposed-24);
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){pitch_[row]=static_cast<std::uint8_t>(proposed);notifyViews(false);}
 return ok;
}
bool Controller::adjustRowMix(std::size_t row,bool adjustPan,int delta) {
 if(row>=kRows)return false;
 auto& values=adjustPan?pan_:gain_;
 const auto proposed=std::clamp(static_cast<std::int64_t>(values[row])+delta,std::int64_t{0},std::int64_t{200});
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowMix");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 attrs->setInt("Gain",adjustPan?gain_[row]:proposed);
 attrs->setInt("Pan",adjustPan?proposed-100:static_cast<int>(pan_[row])-100);
 const bool accepted=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();if(accepted){values[row]=static_cast<std::uint8_t>(proposed);notifyViews(false);}
 return accepted;
}
bool Controller::toggleRowGate(std::size_t row,bool solo) {
 if(row>=kRows)return false;
 auto& gates=solo?solo_:muted_;
 auto* msg=allocateMessage();
 if(!msg)return false;
 msg->setMessageID("Drumator.RowGate");
 auto* attr=msg->getAttributes();
 if(!attr){msg->release();return false;}
 const bool next=!gates[row];
 attr->setInt("Row",static_cast<Steinberg::int64>(row));
 attr->setInt("Solo",solo?1:0);
 attr->setInt("On",next?1:0);
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){gates[row]=next;notifyViews(false);}
 return ok;
}
bool Controller::toggleStep(std::size_t p,std::size_t r,std::size_t s) {
 if(p>=kPatterns||r>=kRows||s>=kSteps)return false;
 return setStepVelocity(p,r,s,velocity(p,r,s)>0?0:100);
}
Controller::LoadStatus Controller::stepSample(std::size_t row,int direction) {
 if(row>=kRows)return LoadStatus::InvalidRow;
 if(direction!=1&&direction!=-1)return LoadStatus::Rejected;
 if(samplePaths_[row].empty())return LoadStatus::NotFound;
 try {
   const auto current=pathFromUtf8(samplePaths_[row]);
   const auto parent=current.parent_path();
   if(parent.empty()||!std::filesystem::is_directory(parent))return LoadStatus::NotFound;
   std::vector<std::string> candidates;
   for(const auto& entry:std::filesystem::directory_iterator(parent)) {
     std::error_code ec;
     if(!entry.is_regular_file(ec)||ec)continue;
     auto ext=pathToUtf8(entry.path().extension());
     std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
     if(ext==".wav"||ext==".flac")candidates.push_back(pathToUtf8(entry.path()));
   }
   if(candidates.size()<2)return LoadStatus::NotFound;
   std::string cursor=pathToUtf8(current);
   // Attempt each candidate at most once, skipping corrupt or inaccessible files.
   // The active sample/path is changed only by a successful loadWav().
   for(std::size_t tried=0;tried<candidates.size()-1;++tried) {
      std::string selected;
      if(!adjacentSample(candidates,cursor,direction,selected))
         return LoadStatus::NotFound;
      const auto status=loadWav(row,selected);
      if(status==LoadStatus::Accepted)return status;
      // A disconnected processor or allocation failure must not be hidden.
      if(status==LoadStatus::NoProcessorConnection||
         status==LoadStatus::Rejected)return status;
      cursor=selected;
   }
   return LoadStatus::NotFound;
 }catch(const std::filesystem::filesystem_error&){return LoadStatus::ReadError;}
  catch(const std::bad_alloc&){return LoadStatus::ReadError;}
}
Controller::LoadStatus Controller::loadWav(std::size_t row,const std::string& path) {
 if(row>=kRows)return LoadStatus::InvalidRow;
 if(path.empty())return LoadStatus::NotFound;
 if(path.size()>4096u)return LoadStatus::Rejected;
 try {
 const auto suffix=pathToUtf8(pathFromUtf8(path).extension());
 std::string ext=suffix;
 std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char ch){
    return static_cast<char>(std::tolower(ch));
 });
 const bool isFlac=ext==".flac";
 if(!isFlac&&ext!=".wav")return LoadStatus::Unsupported;
 constexpr std::uintmax_t kMessageMax=16u*1024u*1024u;
 std::vector<std::uint8_t> data;
 try {
   const auto file=pathFromUtf8(path);
   if(!std::filesystem::is_regular_file(file))return LoadStatus::NotFound;
   const auto length=std::filesystem::file_size(file);
   if(length>kMessageMax)return LoadStatus::TooLarge;
   if(length<(isFlac?16u:44u))return LoadStatus::Unsupported;
   std::ifstream f(file,std::ios::binary);
   if(!f)return LoadStatus::ReadError;
   data.resize(static_cast<std::size_t>(length));
   if(!f.read(reinterpret_cast<char*>(data.data()),
              static_cast<std::streamsize>(data.size())))return LoadStatus::ReadError;
 }catch(const std::filesystem::filesystem_error&) {return LoadStatus::NotFound;}
  catch(const std::exception&) {return LoadStatus::ReadError;}
 if(isFlac) {
    std::vector<std::uint8_t> converted;
    if(!flacToWav(data.data(),data.size(),converted))
      return LoadStatus::Unsupported;
    data.swap(converted);
 }
 DecodedWav preflight;
 if(WavPcm16Decoder::decode(data.data(),data.size(),preflight)!=WavError::Ok)
    return LoadStatus::Unsupported;
 // VST3 hosts are allowed to expose a controller without a connected
 // processor. Do not display "loaded" in that case.
 if(!getPeer())return LoadStatus::NoProcessorConnection;
 const auto last=path.find_last_of("/\\");
 std::string name;
 std::string stagedPath;
 try {
   name=last==std::string::npos?path:path.substr(last+1);
   stagedPath=path;
 }catch(const std::bad_alloc&) {return LoadStatus::ReadError;}
 if(name.empty()||name.size()>kMaxKitName||name.find('\0')!=std::string::npos)
    return LoadStatus::Rejected;
 // A successful Processor commit must never later be reported as failure.
 // Establish IMessage ownership before any potentially allocating SDK call.
 auto* msg=allocateMessage();
 if(!msg)return LoadStatus::NoProcessorConnection;
 struct MessageOwner {
   Steinberg::Vst::IMessage* message;
   ~MessageOwner(){if(message)message->release();}
 } owner{msg};
 msg->setMessageID("Drumator.LoadPcm16Wav");
 auto* attr=msg->getAttributes();
 if(!attr||attr->setInt("Row",static_cast<Steinberg::int64>(row))!=Steinberg::kResultOk||
    attr->setBinary("SampleName",name.data(),
                    static_cast<Steinberg::uint32>(name.size()))!=Steinberg::kResultOk||
    attr->setBinary("WavBytes",data.data(),
                    static_cast<Steinberg::uint32>(data.size()))!=Steinberg::kResultOk)
   return LoadStatus::Rejected;
 if(sendEditMessage(msg)!=Steinberg::kResultOk)return LoadStatus::Rejected;
 // Success means the processor accepted the immutable asset for the
 // next audio block; no claim is made that the buffer has already rendered.
 labels_[row].swap(name);
 samplePaths_[row].swap(stagedPath);
 samplePresent_[row]=true;
 originIdentity_[row]=wavIdentity(data);
 originIdentityValid_[row]=true;
 pathsRestoredBeforeComponent_=false;
 componentStatePendingController_=false;
 // The embedded WAV and UI metadata are committed. Host repaint/dirty
 // notification failures must not turn that accepted import into ReadError.
 try {notifyViews(false);}catch(...) { /* committed; next repaint refreshes */ }
 return LoadStatus::Accepted;
 }catch(const std::filesystem::filesystem_error&) {return LoadStatus::NotFound;}
  catch(const std::exception&) {return LoadStatus::ReadError;}
}
}
