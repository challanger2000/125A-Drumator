#include "DrumatorController.h"
#include "PluginIDs.h"
#include "core/SequencerCore.h"
#include "base/source/fstreamer.h"
namespace drumator {
namespace {
constexpr Steinberg::uint32 kStateMagic=0x31524D44u;
}
Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context) {
 const auto result=EditController::initialize(context);
 if(result!=Steinberg::kResultOk)return result;
 parameters.addParameter(STR16("Pattern"),nullptr,7,0.0,
                         Steinberg::Vst::ParameterInfo::kCanAutomate,kPatternParam);
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Controller::setComponentState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,Steinberg::kLittleEndian);
 Steinberg::uint32 magic=0,pattern=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(pattern)||
    magic!=kStateMagic||pattern>=kPatterns)
    return Steinberg::kResultFalse;
 return setParamNormalized(kPatternParam,static_cast<double>(pattern)/7.0);
}
}
