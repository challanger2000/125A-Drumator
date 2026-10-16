#include "DrumatorController.h"
#include "DrumatorView.h"
#include "PluginIDs.h"
#include "core/WavPcm16Decoder.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstattributes.h"
#include "vstgui/lib/cstring.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace drumator {
namespace {
constexpr Steinberg::uint32 kOldState=0x31524D44u;
constexpr Steinberg::uint32 kNewState=0x32524D44u;
}
Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context) {
 const auto result=EditController::initialize(context);
 if(result!=Steinberg::kResultOk)return result;
 parameters.addParameter(STR16("Pattern"),nullptr,7,0.0,
                         Steinberg::Vst::ParameterInfo::kCanAutomate,kPatternParam);
 // Provisional processor factory rhythm on row one for integration tests.
 for(std::size_t p=0;p<kPatterns;++p)
   for(std::size_t step=0;step<kSteps;step+=4)
     grid_[(p*kRows)*kSteps+step]=(step%8==0)?127:90;
 for(auto& name:labels_)name="LEER - WAV LADEN";
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Controller::setComponentState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 Steinberg::uint32 magic=0,pattern=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(pattern)||pattern>=kPatterns)
    return Steinberg::kResultFalse;
 if(magic==kNewState) {
   std::array<std::uint8_t,kCells> candidate{};
   for(auto& step:candidate)
       if(!stream.readInt8u(step)||step>127)return Steinberg::kResultFalse;
   grid_=candidate;
 } else if(magic!=kOldState) {
   return Steinberg::kResultFalse;
 }
 return setParamNormalized(kPatternParam,static_cast<double>(pattern)/7.0);
}
Steinberg::IPlugView* PLUGIN_API Controller::createView(Steinberg::FIDString name) {
 if(name&&std::strcmp(name,Steinberg::Vst::ViewType::kEditor)==0) {
    auto* editor=new VSTGUI::VST3Editor(this,"DrumatorView","Drumator.uidesc");
    editor->setAllowedZoomFactors({1.0,1.5});
    return editor;
 }
 return nullptr;
}
VSTGUI::CView* Controller::createCustomView(VSTGUI::UTF8StringPtr name,
          const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
          VSTGUI::VST3Editor* editor) {
 if(!name||std::strcmp(name,"DrumatorStepGrid")!=0)return nullptr;
 return new DrumatorView(VSTGUI::CRect(0,0,1100,600),*this,*editor);
}
std::uint8_t Controller::velocity(std::size_t p,std::size_t r,std::size_t s) const noexcept {
 if(p>=kPatterns||r>=kRows||s>=kSteps)return 0;
 return grid_[(p*kRows+r)*kSteps+s];
}
std::size_t Controller::currentPattern() noexcept {
 const double value=getParamNormalized(kPatternParam);
 if(!std::isfinite(value))return 0;
 return static_cast<std::size_t>(std::lround(std::clamp(value,0.0,1.0)*7.0));
}
const std::string& Controller::label(std::size_t row) const noexcept {
 static const std::string empty;
 return row<kRows?labels_[row]:empty;
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
bool Controller::toggleStep(std::size_t p,std::size_t r,std::size_t s) {
 if(p>=kPatterns||r>=kRows||s>=kSteps)return false;
 const auto next=velocity(p,r,s)>0?0:127;
 auto* msg=allocateMessage();
 if(!msg)return false;
 msg->setMessageID("Drumator.StepEdit");
 auto* attr=msg->getAttributes();
 if(!attr){msg->release();return false;}
 attr->setInt("Pattern",static_cast<Steinberg::int64>(p));
 attr->setInt("Row",static_cast<Steinberg::int64>(r));
 attr->setInt("Step",static_cast<Steinberg::int64>(s));
 attr->setInt("Velocity",static_cast<Steinberg::int64>(next));
 const auto ok=sendMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(ok)grid_[(p*kRows+r)*kSteps+s]=static_cast<std::uint8_t>(next);
 return ok;
}
Controller::LoadStatus Controller::loadWav(std::size_t row,const std::string& path) {
 if(row>=kRows)return LoadStatus::InvalidRow;
 if(path.empty())return LoadStatus::NotFound;
 constexpr std::uintmax_t kMessageMax=16u*1024u*1024u;
 std::vector<std::uint8_t> data;
 try {
   const auto file=std::filesystem::u8path(path);
   if(!std::filesystem::is_regular_file(file))return LoadStatus::NotFound;
   const auto length=std::filesystem::file_size(file);
   if(length>kMessageMax)return LoadStatus::TooLarge;
   if(length<44)return LoadStatus::Unsupported;
   std::ifstream f(file,std::ios::binary);
   if(!f)return LoadStatus::ReadError;
   data.resize(static_cast<std::size_t>(length));
   if(!f.read(reinterpret_cast<char*>(data.data()),
              static_cast<std::streamsize>(data.size())))return LoadStatus::ReadError;
 }catch(const std::filesystem::filesystem_error&) {return LoadStatus::NotFound;}
  catch(const std::exception&) {return LoadStatus::ReadError;}
 DecodedWav preflight;
 if(WavPcm16Decoder::decode(data.data(),data.size(),preflight)!=WavError::Ok)
    return LoadStatus::Unsupported;
 // VST3 hosts are allowed to expose a controller without a connected
 // processor. Do not display "loaded" in that case.
 if(!getPeer())return LoadStatus::NoProcessorConnection;
 auto* msg=allocateMessage();
 if(!msg)return LoadStatus::NoProcessorConnection;
 msg->setMessageID("Drumator.LoadPcm16Wav");
 auto* attr=msg->getAttributes();
 if(!attr){msg->release();return LoadStatus::Rejected;}
 attr->setInt("Row",static_cast<Steinberg::int64>(row));
 if(attr->setBinary("WavBytes",data.data(),
                    static_cast<Steinberg::uint32>(data.size()))!=Steinberg::kResultOk) {
     msg->release();return LoadStatus::Rejected;
 }
 const bool accepted=sendMessage(msg)==Steinberg::kResultOk;
 msg->release();
 if(!accepted)return LoadStatus::Rejected;
 // Success means the processor accepted the immutable asset for the
 // next audio block; no claim is made that the buffer has already rendered.
 const auto last=path.find_last_of("/\\");
 labels_[row]=last==std::string::npos?path:path.substr(last+1);
 return LoadStatus::Accepted;
}
}
