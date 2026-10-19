#include "DrumatorController.h"
#include "DrumatorView.h"
#include "DrumatorEditor.h"
#include "PluginIDs.h"
#include "core/WavPcm16Decoder.h"
#include "core/FlacImport.h"
#include "core/StepVelocity.h"
#include "core/HostEditDirty.h"
#include "core/PatternQuantization.h"
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
#include "core/SampleFolderNavigation.h"
#include <fstream>
#include <vector>

namespace drumator {
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
}
void Controller::notifyViews(bool relayout) {
 for(auto* view:views_)if(view)view->refreshFromHost(relayout);
}
Steinberg::tresult PLUGIN_API Controller::setParamNormalized(
    Steinberg::Vst::ParamID id,Steinberg::Vst::ParamValue value) {
 const auto result=EditController::setParamNormalized(id,value);
 if(result==Steinberg::kResultOk&&id==kPatternParam)notifyViews(false);
 return result;
}
Steinberg::tresult Controller::sendEditMessage(Steinberg::Vst::IMessage* message) {
 const auto result=sendMessage(message);
 markSuccessfulHostEdit(result==Steinberg::kResultOk,[&](){
   if(componentHandler2)componentHandler2->setDirty(true);
 });
 return result;
}
namespace {
constexpr Steinberg::uint32 kZoomControllerState=0x314D5A44u; // DZM1
constexpr Steinberg::uint32 kOldState=0x31524D44u;
constexpr Steinberg::uint32 kNewState=0x32524D44u;
constexpr auto kRowsState=kStateMagicV3;
}
Steinberg::tresult PLUGIN_API Controller::getState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 return stream.writeInt32u(kZoomControllerState)&&
        stream.writeInt8u(preferredZoom_>1.25?1:0)
     ? Steinberg::kResultOk : Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Controller::setState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 Steinberg::uint32 magic=0;
 Steinberg::uint8 zoom=0;
 if(!stream.readInt32u(magic)||magic!=kZoomControllerState||
    !stream.readInt8u(zoom)||zoom>1)return Steinberg::kResultFalse;
 preferredZoom_=zoom?1.5:1.0;
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
 // Provisional processor factory rhythm on row one for integration tests.
 for(std::size_t p=0;p<kPatterns;++p)
   for(std::size_t step=0;step<kSteps;step+=4)
     grid_[(p*kRows)*kSteps+step]=(step%8==0)?127:90;
 for(auto& name:labels_)name="LEER - WAV LADEN";
 gain_.fill(100);pan_.fill(100);pitch_.fill(24);choke_.fill(0);
 for(std::size_t row=0;row<kRows;++row)output_[row]=static_cast<std::uint8_t>(row);
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Controller::setComponentState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 Steinberg::uint32 magic=0,next=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(next))
    return Steinberg::kResultFalse;
 if(magic==kKitMagic||magic==kKitMagicV13||magic==kKitMagicV12||magic==kKitMagicV11||magic==kKitMagicV10||magic==kKitMagicV9||magic==kKitMagicV8||magic==kKitMagicV7||magic==kKitMagicV6||magic==kKitMagicV5||magic==kKitMagicV4) {
    if(next<2+kCells+kRows*6+4||next>kMaxKitBytes-8)
       return Steinberg::kResultFalse;
    try {
       std::vector<std::uint8_t> encoded(8+static_cast<std::size_t>(next));
       for(int i=0;i<4;++i) {
           encoded[i]=static_cast<std::uint8_t>(magic>>(8*i));
           encoded[4+i]=static_cast<std::uint8_t>(next>>(8*i));
       }
       KitState candidate;
       if(stream.readRaw(encoded.data()+8,next)!=static_cast<Steinberg::TSize>(next)||
          !KitCodec::decode(encoded.data(),encoded.size(),candidate))
          return Steinberg::kResultFalse;
       grid_=candidate.grid;
       customNames_=candidate.displayNames;
       undo_.reset();
       clipboardValid_=false;
       samplePaths_.fill({});
       visibleRows_=candidate.visible;
       swingPercent_=candidate.swing;
       humanizePercent_=candidate.humanize;
       masterVolume_=candidate.masterVolume;
       randomVelocity_=candidate.randomVelocity;
       songEnabled_=candidate.songEnabled!=0;
       songChain_=candidate.chain;
       for(std::size_t row=0;row<kRows;++row) {
          muted_[row]=(candidate.muted&(1u<<row))!=0;
          solo_[row]=(RowVisibility::visibleSolo(candidate.solo,candidate.visible)&
                      (1u<<row))!=0;
          gain_[row]=candidate.gain[row];pan_[row]=candidate.pan[row];
          pitch_[row]=candidate.pitch[row];choke_[row]=candidate.choke[row];output_[row]=candidate.output[row];
       }
       for(std::size_t row=0;row<kRows;++row)
          labels_[row]=candidate.names[row].empty()
                        ?"LEER - WAV LADEN":candidate.names[row];
       const auto result=setParamNormalized(kPatternParam,
                  static_cast<double>(candidate.selected)/7.0);
       if(result==Steinberg::kResultOk)notifyViews(true);
       return result;
    }catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
 }
 if(next>=kPatterns)return Steinberg::kResultFalse;
 std::uint8_t candidateRows=static_cast<std::uint8_t>(kRows);
 if(magic==kRowsState) {
   if(!stream.readInt8u(candidateRows)||!RowVisibility::valid(candidateRows))
      return Steinberg::kResultFalse;
 }
 if(magic==kNewState||magic==kRowsState) {
   std::array<std::uint8_t,kCells> candidate{};
   for(auto& step:candidate)
       if(!stream.readInt8u(step)||step>127)return Steinberg::kResultFalse;
   grid_=candidate;
 }else if(magic==kOldState) {
   // DMR1 contains no grid; restore the identical factory pattern used by
   // the processor instead of retaining edits from the preceding project.
   grid_.fill(0);
   for(std::size_t p=0;p<kPatterns;++p)
     for(std::size_t step=0;step<kSteps;step+=4)
       grid_[(p*kRows)*kSteps+step]=(step%8==0)?127:90;
 }else return Steinberg::kResultFalse;
 visibleRows_=candidateRows;
 customNames_.fill({});
 undo_.reset();
 clipboardValid_=false;
 samplePaths_.fill({});
 muted_.fill(false);solo_.fill(false);swingPercent_=0;humanizePercent_=0;
 randomVelocity_.fill(0);masterVolume_=100;
 songEnabled_=false;songChain_=SongChain{};
 gain_.fill(100);pan_.fill(100);pitch_.fill(24);choke_.fill(0);
 for(std::size_t row=0;row<kRows;++row)output_[row]=static_cast<std::uint8_t>(row);
 for(auto& name:labels_)name="LEER - WAV LADEN";
 const auto result=setParamNormalized(kPatternParam,static_cast<double>(next)/7.0);
 if(result==Steinberg::kResultOk)notifyViews(true);
 return result;
}
Steinberg::IPlugView* PLUGIN_API Controller::createView(Steinberg::FIDString name) {
 if(name&&std::strcmp(name,Steinberg::Vst::ViewType::kEditor)==0) {
    auto* editor=new DrumatorEditor(this,"DrumatorView","Drumator.uidesc");
    editor->setAllowedZoomFactors({1.0,1.5});
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
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RenameRow");
 auto* attrs=msg->getAttributes();
 if(!attrs||attrs->setInt("Row",static_cast<Steinberg::int64>(row))!=Steinberg::kResultOk||
    attrs->setBinary("DisplayName",name.data(),static_cast<Steinberg::uint32>(name.size()))!=Steinberg::kResultOk){
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){customNames_[row]=name;notifyViews(false);}
 return ok;
}
bool Controller::configureSong(bool enabled,std::size_t count,const SongChain& chain) {
 if(count==0||count>SongChain::kCapacity||chain.count()!=count)return false;
 std::array<std::uint8_t,2*SongChain::kCapacity> bytes{};
 for(std::size_t i=0;i<count;++i){
   const auto entry=chain.entry(i);
   if(entry.pattern>=kPatterns||!SongChain::validLength(entry.steps))return false;
   bytes[2*i]=entry.pattern;bytes[2*i+1]=entry.steps;
 }
 auto* msg=allocateMessage();
 if(!msg)return false;
 msg->setMessageID("Drumator.SongConfig");
 auto* a=msg->getAttributes();
 if(!a||a->setInt("Enabled",enabled?1:0)!=Steinberg::kResultOk||
    a->setInt("Count",static_cast<Steinberg::int64>(count))!=Steinberg::kResultOk||
    a->setBinary("Entries",bytes.data(),static_cast<Steinberg::uint32>(count*2))!=Steinberg::kResultOk){
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){songEnabled_=enabled;songChain_=chain;notifyViews(false);}
 return ok;
}
bool Controller::toggleSongMode(){return configureSong(!songEnabled_,songChain_.count(),songChain_);}
bool Controller::adjustSongCount(int delta){
 const auto old=static_cast<int>(songChain_.count());
 const auto next=std::clamp(old+delta,1,static_cast<int>(SongChain::kCapacity));
 SongChain candidate=songChain_;
 if(!candidate.setCount(static_cast<std::size_t>(next)))return false;
 return configureSong(songEnabled_,candidate.count(),candidate);
}
bool Controller::setSongEntry(std::size_t slot,std::uint8_t pattern,std::uint8_t steps){
 if(slot>=songChain_.count()||pattern>=kPatterns||!SongChain::validLength(steps))return false;
 SongChain candidate=songChain_;
 if(!candidate.configure(slot,pattern,steps))return false;
 return configureSong(songEnabled_,candidate.count(),candidate);
}
bool Controller::editSongEntry(std::size_t slot,int patternDelta,int lengthDelta){
 if(slot>=songChain_.count())return false;
 SongChain candidate=songChain_;
 const auto old=candidate.entry(slot);
 const auto pattern=static_cast<std::uint8_t>((static_cast<int>(old.pattern)+patternDelta+8)%8);
 const std::array<std::uint8_t,3> lengths{8,16,32};
 const auto index=old.steps==8?0:(old.steps==16?1:2);
 const auto length=lengths[static_cast<std::size_t>((index+lengthDelta+3)%3)];
 if(!candidate.configure(slot,pattern,length))return false;
 return configureSong(songEnabled_,candidate.count(),candidate);
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
bool Controller::replacePattern(std::size_t p,const PatternSteps& data,bool remember) {
 if(p>=kPatterns)return false;
 PatternSteps old{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,old.size(),old.begin());
 if(old==data)return true;
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.PatternReplace");
 auto* a=msg->getAttributes();
 if(!a||a->setInt("Pattern",static_cast<Steinberg::int64>(p))!=Steinberg::kResultOk||
    a->setBinary("Steps",data.data(),static_cast<Steinberg::uint32>(data.size()))!=Steinberg::kResultOk){
   msg->release();return false;
 }
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){
   if(remember)undo_.capture(p,old,data);
   std::copy(data.begin(),data.end(),grid_.begin()+p*kRows*kSteps);
   notifyViews(false);
 }
 return ok;
}
bool Controller::copyCurrentPattern() {
 const auto p=currentPattern();
 std::copy_n(grid_.begin()+p*kRows*kSteps,clipboard_.size(),clipboard_.begin());
 clipboardValid_=true;return true;
}
bool Controller::pasteCurrentPattern(){return clipboardValid_&&replacePattern(currentPattern(),clipboard_);}
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
 if(!replacePattern(destination,data))return false;
 const bool switched=changePattern(destination);
 if(selectedCopy)*selectedCopy=switched;
 return true; // the copy exists even if host refuses selecting it
}
bool Controller::clearCurrentPattern(){return replacePattern(currentPattern(),{});}
bool Controller::undoCurrentPattern(){
 const auto p=currentPattern();
 const auto* old=undo_.snapshot(p);
 if(!old)return false;
 if(!replacePattern(p,*old,false))return false;
 undo_.consume(p);
 return true;
}
bool Controller::varyPatternVelocity(){
 const auto p=currentPattern();
 PatternSteps data{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,data.size(),data.begin());
 // Deterministic per edit: changes stored velocities only, never note placement.
 const auto variation=velocityVariation_+1u;
 for(std::size_t row=0;row<kRows;++row)
  for(std::size_t step=0;step<kSteps;++step){
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
 PatternSteps data{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,data.size(),data.begin());
 for(auto& value:data)if(value!=0)value=100;
 return replacePattern(p,data);
}
bool Controller::fillRow(std::size_t row,RowFill mode){
 if(row>=visibleRows_)return false;
 const auto p=currentPattern();
 PatternSteps data{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,data.size(),data.begin());
 return replacePattern(p,fillInstrument(data,row,mode));
}
bool Controller::canHideLast() const noexcept {
 if(visibleRows_<=1||visibleRows_>kRows)return false;
 const auto row=static_cast<std::size_t>(visibleRows_-1);
 if(labels_[row]!="LEER - WAV LADEN"||!customNames_[row].empty()||solo_[row])return false;
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
 if(good)++visibleRows_;
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
 if(good)visibleRows_=static_cast<std::uint8_t>(previous);
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
 if(good)--visibleRows_;
 return good;
}
void Controller::rememberUndoStep(std::size_t p,std::size_t r,std::size_t s,std::uint8_t next) {
 PatternSteps old{};
 std::copy_n(grid_.begin()+p*kRows*kSteps,old.size(),old.begin());
 auto changed=old;changed[r*kSteps+s]=next;
 undo_.capture(p,old,changed);
}
bool Controller::setStepVelocity(std::size_t p,std::size_t r,std::size_t s,std::uint8_t next){
 if(p>=kPatterns||r>=kRows||s>=kSteps||next>127)return false;
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
   rememberUndoStep(p,r,s,next);
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
 const auto value=std::clamp(static_cast<int>(randomVelocity_[row])+delta,0,100);
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
 const auto value=std::clamp(static_cast<int>(masterVolume_)+delta,0,200);
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
 const auto next=std::clamp(static_cast<int>(humanizePercent_)+delta,0,100);
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.Humanize");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Percent",next);
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();if(ok)humanizePercent_=static_cast<std::uint8_t>(next);
 return ok;
}
bool Controller::adjustSwing(int delta) {
 const auto next=std::clamp(static_cast<int>(swingPercent_)+delta,0,50);
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.Swing");
 auto* attr=msg->getAttributes();if(!attr){msg->release();return false;}
 attr->setInt("Percent",next);
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok)swingPercent_=static_cast<std::uint8_t>(next);
 return ok;
}
bool Controller::previewSample(std::size_t row) {
 if(row>=visibleRows_)return false;
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.PreviewRow");
 auto* attrs=msg->getAttributes();
 if(!attrs||attrs->setInt("Row",static_cast<Steinberg::int64>(row))!=Steinberg::kResultOk) {
   msg->release();return false;
 }
 // A preview must not mark the DAW project dirty.
 const auto result=sendMessage(msg);
 msg->release();
 return result==Steinberg::kResultOk;
}
bool Controller::clearSample(std::size_t row) {
 if(row>=kRows)return false;
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.ClearSample");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok){labels_[row]="LEER - WAV LADEN";samplePaths_[row].clear();}
 return ok;
}
bool Controller::cycleRowOutput(std::size_t row) {
 if(row>=kRows)return false;
 const auto bus=static_cast<std::uint8_t>((output_[row]+1)%kRows);
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowOutput");
 auto* attrs=msg->getAttributes();
 if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 attrs->setInt("Bus",static_cast<Steinberg::int64>(bus));
 const bool accepted=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();if(accepted)output_[row]=bus;
 return accepted;
}
bool Controller::cycleRowChoke(std::size_t row,int direction) {
 if(row>=kRows||(direction!=1&&direction!=-1))return false;
 const auto next=static_cast<std::uint8_t>((static_cast<int>(choke_[row])+5+direction)%5);
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowChoke");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 attrs->setInt("Group",next);
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok)choke_[row]=next;
 return ok;
}
bool Controller::adjustRowPitch(std::size_t row,int delta) {
 if(row>=kRows)return false;
 const int proposed=std::clamp(static_cast<int>(pitch_[row])+delta,0,48);
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowPitch");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 attrs->setInt("Semitones",proposed-24);
 const bool ok=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok)pitch_[row]=static_cast<std::uint8_t>(proposed);
 return ok;
}
bool Controller::adjustRowMix(std::size_t row,bool adjustPan,int delta) {
 if(row>=kRows)return false;
 auto& values=adjustPan?pan_:gain_;
 const auto proposed=std::clamp(static_cast<int>(values[row])+delta,0,200);
 auto* msg=allocateMessage();if(!msg)return false;
 msg->setMessageID("Drumator.RowMix");
 auto* attrs=msg->getAttributes();if(!attrs){msg->release();return false;}
 attrs->setInt("Row",static_cast<Steinberg::int64>(row));
 attrs->setInt("Gain",adjustPan?gain_[row]:proposed);
 attrs->setInt("Pan",adjustPan?proposed-100:static_cast<int>(pan_[row])-100);
 const bool accepted=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();if(accepted)values[row]=static_cast<std::uint8_t>(proposed);
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
 if(ok)gates[row]=next;
 return ok;
}
bool Controller::toggleStep(std::size_t p,std::size_t r,std::size_t s) {
 if(p>=kPatterns||r>=kRows||s>=kSteps)return false;
 return setStepVelocity(p,r,s,velocity(p,r,s)>0?0:127);
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
 const auto name=last==std::string::npos?path:path.substr(last+1);
 if(name.empty()||name.size()>kMaxKitName||name.find('\0')!=std::string::npos)
    return LoadStatus::Rejected;
 auto* msg=allocateMessage();
 if(!msg)return LoadStatus::NoProcessorConnection;
 msg->setMessageID("Drumator.LoadPcm16Wav");
 auto* attr=msg->getAttributes();
 if(!attr){msg->release();return LoadStatus::Rejected;}
 attr->setInt("Row",static_cast<Steinberg::int64>(row));
 if(attr->setBinary("SampleName",name.data(),
                    static_cast<Steinberg::uint32>(name.size()))!=Steinberg::kResultOk) {
    msg->release();return LoadStatus::Rejected;
 }
 if(attr->setBinary("WavBytes",data.data(),
                    static_cast<Steinberg::uint32>(data.size()))!=Steinberg::kResultOk) {
     msg->release();return LoadStatus::Rejected;
 }
 const bool accepted=sendEditMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(!accepted)return LoadStatus::Rejected;
 // Success means the processor accepted the immutable asset for the
 // next audio block; no claim is made that the buffer has already rendered.
 labels_[row]=name;
 samplePaths_[row]=path;
 return LoadStatus::Accepted;
}
}
