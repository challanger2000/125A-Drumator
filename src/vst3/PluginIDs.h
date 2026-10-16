#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
namespace drumator {
static const Steinberg::FUID kProcessorCID(0x125AD123,0xB00D431A,0xA7F35E48,0x95019021);
static const Steinberg::FUID kControllerCID(0x125AD123,0xB00D431A,0xA7F35E48,0x95019022);
constexpr Steinberg::Vst::ParamID kPatternParam=1000;
constexpr const char* kPluginVersion="0.1.0";
}
