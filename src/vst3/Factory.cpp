#include "DrumatorProcessor.h"
#include "DrumatorController.h"
#include "PluginIDs.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

BEGIN_FACTORY_DEF("125A Systems",
                  "https://github.com/challanger2000/125A-Drumator","")

DEF_CLASS2(INLINE_UID_FROM_FUID(drumator::kProcessorCID),
           Steinberg::PClassInfo::kManyInstances,
           kVstAudioEffectClass,
           "125A Drumator",
           Steinberg::Vst::kDistributable,
           Steinberg::Vst::PlugType::kInstrumentDrum,
           drumator::kPluginVersion,
           kVstVersionString,
           drumator::Processor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(drumator::kControllerCID),
           Steinberg::PClassInfo::kManyInstances,
           kVstComponentControllerClass,
           "125A Drumator Controller",0,"",
           drumator::kPluginVersion,
           kVstVersionString,
           drumator::Controller::createInstance)

END_FACTORY
