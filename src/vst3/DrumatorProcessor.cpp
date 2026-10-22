#include "DrumatorProcessor.h"
#include "core/PatternQuantization.h"
#include "PluginIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstattributes.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <memory>

namespace drumator {
namespace {
constexpr double kPi=3.141592653589793238462643383279502884;
// Distinct, bounded processor state format (not the future pattern bank format).
constexpr Steinberg::uint32 kStateMagic=0x31524D44u; // legacy "DMR1"
constexpr Steinberg::uint32 kStateMagicV2=0x32524D44u; // "DMR2", 4096 step bytes
constexpr auto kStateMagicV3Local=kStateMagicV3; // plus visible row count

}
Processor::Processor() {
 setControllerClass(kControllerCID);
 for(auto& length:patternLengths_)length.store(32,std::memory_order_relaxed);
 for(std::size_t row=0;row<kRows;++row)rowOutput_[row].store(static_cast<std::uint8_t>(row));
 // The initial test pattern is supplied by AtomicPatternBank.
}
Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context) {
 const auto result=AudioEffect::initialize(context);
 if(result!=Steinberg::kResultOk)return result;
 addAudioOutput(STR16("MAIN"),Steinberg::Vst::SpeakerArr::kStereo);
 // Studio One shows kAux VST3 output buses but may refuse to enable them.
 // Report all independent instrument output buses as kMain (Steinberg
 // developer forum); still request stems inactive at startup via flags=0.
 // This does not modify voice routing, project state, or bus order.
 for(int row=1;row<static_cast<int>(kRows);++row) {
   Steinberg::Vst::String128 name{};
   const auto* prefix=STR16("DRUM ");
   for(int n=0;n<5;++n)name[n]=prefix[n];
   const int number=row+1;
   name[5]=static_cast<Steinberg::char16>('0'+number/10);
   name[6]=static_cast<Steinberg::char16>('0'+number%10);
   addAudioOutput(name,Steinberg::Vst::SpeakerArr::kStereo,
                  Steinberg::Vst::BusTypes::kMain,0);
 }
 addEventInput(STR16("MIDI Input"),16);
 processContextRequirements.needProjectTimeMusic().needTempo().needTransportState();
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::setBusArrangements(
 Steinberg::Vst::SpeakerArrangement* inputs,Steinberg::int32 numIns,
 Steinberg::Vst::SpeakerArrangement* outputs,Steinberg::int32 numOuts) {
 (void)inputs;
 // Bus *activation* is independent of arrangement negotiation. We expose
 // exactly 16 fixed stereo buses and do not implement bus-count resizing.
 // Advertising success for a shorter array falsely tells a host that its
 // requested I/O configuration is installed, although getBusCount stays 16.
 if(numIns!=0||numOuts!=static_cast<Steinberg::int32>(kRows)||
    !outputs)return Steinberg::kResultFalse;
 for(Steinberg::int32 i=0;i<numOuts;++i)
   if(outputs[i]!=Steinberg::Vst::SpeakerArr::kStereo)
      return Steinberg::kResultFalse;
 // Fixed stereo arrangements; optional stem buses are activated by host.
 return Steinberg::kResultTrue;
}
Steinberg::tresult PLUGIN_API Processor::activateBus(
 Steinberg::Vst::MediaType type,Steinberg::Vst::BusDirection dir,
 Steinberg::int32 index,Steinberg::TBool state) {
 const auto result=AudioEffect::activateBus(type,dir,index,state);
 if(result!=Steinberg::kResultOk)return result;
 if(type==Steinberg::Vst::MediaTypes::kAudio&&
    dir==Steinberg::Vst::BusDirections::kOutput&&
    index>0&&index<static_cast<Steinberg::int32>(kRows)) {
    const auto bit=std::uint32_t{1}<<static_cast<unsigned>(index);
    if(state)activeStemMask_.fetch_or(bit,std::memory_order_relaxed);
    else activeStemMask_.fetch_and(~bit,std::memory_order_relaxed);
 }
 return result;
}
Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(Steinberg::int32 size) {
 return size==Steinberg::Vst::kSample32?Steinberg::kResultTrue:Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool state) {
 if(!state) {voices_.reset();wasPlaying_=false;haveExpectedPpq_=false;
             previewPending_.store(0,std::memory_order_release);}
 return AudioEffect::setActive(state);
}
Steinberg::tresult PLUGIN_API Processor::setProcessing(Steinberg::TBool state) {
 // Steinberg's AudioEffect default returns kNotImplemented. A real host
 // may invoke this independently of setActive; honor the lifecycle.
 if(!state) {voices_.reset();wasPlaying_=false;haveExpectedPpq_=false;
             previewPending_.store(0,std::memory_order_release);}
 return Steinberg::kResultOk;
}
// Messages are delivered on the UI thread by the VST3 IConnectionPoint
// contract, never by process(). The controller will eventually send the
// bounded PCM16 WAV payload after a user picks a sample.
Steinberg::tresult PLUGIN_API Processor::notify(Steinberg::Vst::IMessage* msg) {
 if(!msg||!msg->getMessageID())return AudioEffect::notify(msg);
 const auto* id=msg->getMessageID();
 // Control-thread-only state query. The audio selection is atomic; no
 // callback lock, heap work or mutation is required.
 if(std::strcmp(id,"Drumator.QueryPattern")==0) {
   auto* attributes=msg->getAttributes();
   return attributes&&attributes->setInt("Selected",
       static_cast<Steinberg::int64>(selected_.load(std::memory_order_acquire)))==Steinberg::kResultOk
       ? Steinberg::kResultOk : Steinberg::kResultFalse;
 }

 if(std::strcmp(id,"Drumator.LoadPcm16Wav")!=0&&
    std::strcmp(id,"Drumator.StepEdit")!=0&&
    std::strcmp(id,"Drumator.PatternReplace")!=0&&
    std::strcmp(id,"Drumator.VisibleRows")!=0&&
    std::strcmp(id,"Drumator.RowGate")!=0&&
    std::strcmp(id,"Drumator.RowMix")!=0&&
    std::strcmp(id,"Drumator.RowPitch")!=0&&
    std::strcmp(id,"Drumator.RowChoke")!=0&&
    std::strcmp(id,"Drumator.RowOutput")!=0&&
    std::strcmp(id,"Drumator.Swing")!=0&&
    std::strcmp(id,"Drumator.Humanize")!=0&&
    std::strcmp(id,"Drumator.RowRandomVelocity")!=0&&
    std::strcmp(id,"Drumator.MasterVolume")!=0&&
    std::strcmp(id,"Drumator.SongConfig")!=0&&
    std::strcmp(id,"Drumator.ClearSample")!=0&&
    std::strcmp(id,"Drumator.RenameRow")!=0&&
    std::strcmp(id,"Drumator.PreviewRow")!=0&&
    std::strcmp(id,"Drumator.PatternLength")!=0)
    return AudioEffect::notify(msg);
 auto* attrs=msg->getAttributes();
 if(!attrs)return Steinberg::kInvalidArgument;
 Steinberg::int64 row=-1;
 if(std::strcmp(id,"Drumator.PatternLength")==0) {
   Steinberg::int64 index=-1,steps=-1;
   if(attrs->getInt("Pattern",index)!=Steinberg::kResultOk||
      attrs->getInt("Steps",steps)!=Steinberg::kResultOk||
      index<0||index>=static_cast<Steinberg::int64>(kPatterns)||
      (steps!=16&&steps!=32))return Steinberg::kInvalidArgument;
   const auto i=static_cast<std::size_t>(index);
   {std::lock_guard<std::mutex> guard(kitMutex_);
    if(!patterns_.setLength(i,static_cast<std::uint8_t>(steps)))return Steinberg::kResultFalse;
    kit_.patternLengths[i]=static_cast<std::uint8_t>(steps);
    patternLengths_[i].store(static_cast<std::uint8_t>(steps),std::memory_order_release);
   }
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.PreviewRow")==0) {
   Steinberg::int64 index=-1;
   if(attrs->getInt("Row",index)!=Steinberg::kResultOk||
      index<0||index>=static_cast<Steinberg::int64>(kRows)||
      index>=static_cast<Steinberg::int64>(visibleRows_.load(std::memory_order_relaxed)))
     return Steinberg::kInvalidArgument;
   std::lock_guard<std::mutex> guard(kitMutex_);
   if(kit_.wav[static_cast<std::size_t>(index)].empty())return Steinberg::kResultFalse;
   previewPending_.fetch_or(std::uint32_t{1}<<static_cast<unsigned>(index),
                            std::memory_order_release);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RenameRow")==0) {
   Steinberg::int64 index=-1;
   const void* bytes=nullptr;
   Steinberg::uint32 len=0;
   if(attrs->getInt("Row",index)!=Steinberg::kResultOk||index<0||
      index>=static_cast<Steinberg::int64>(kRows)||
      attrs->getBinary("DisplayName",bytes,len)!=Steinberg::kResultOk||
      len>kMaxRowLabel||(len!=0&&bytes==nullptr))return Steinberg::kInvalidArgument;
   // Name allocation is entirely in the preparation phase. A host may
   // deliver a 64-byte UTF-8 name while under memory pressure; no exception
   // must escape the VST3 ABI or leave the previous kit label damaged.
   try {
     std::string name(len?static_cast<const char*>(bytes):"",len);
     if(!validRowLabel(name))return Steinberg::kInvalidArgument;
     std::lock_guard<std::mutex> guard(kitMutex_);
     kit_.displayNames[static_cast<std::size_t>(index)].swap(name);
   }catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.ClearSample")==0) {
   Steinberg::int64 index=-1;
   if(attrs->getInt("Row",index)!=Steinberg::kResultOk||
      index<0||index>=static_cast<Steinberg::int64>(kRows))
      return Steinberg::kInvalidArgument;
   const auto rate=outputRate_.load(std::memory_order_acquire);
   if(rate<=0.0||!std::isfinite(rate))return Steinberg::kResultFalse;
   std::unique_ptr<PreparedSample> silent;
   try {silent=PreparedSample::silence(rate);}
   catch(const std::bad_alloc&){return Steinberg::kOutOfMemory;}
   if(!silent)return Steinberg::kResultFalse;
   std::lock_guard<std::mutex> guard(kitMutex_);
   for(;;){auto unused=exchange_.pollReclaim();if(!unused)break;}
   auto old=exchange_.withdraw(static_cast<std::size_t>(index));
   (void)old;
   if(!exchange_.publish(static_cast<std::size_t>(index),silent))
      return Steinberg::kResultFalse;
   previewPending_.fetch_and(~(std::uint32_t{1}<<static_cast<unsigned>(index)),
                             std::memory_order_acq_rel);
   kit_.wav[static_cast<std::size_t>(index)].clear();
   kit_.names[static_cast<std::size_t>(index)].clear();
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.SongConfig")==0) {
   Steinberg::int64 enabled=-1,count=-1;
   if(attrs->getInt("Enabled",enabled)!=Steinberg::kResultOk||
      attrs->getInt("Count",count)!=Steinberg::kResultOk||
      (enabled!=0&&enabled!=1)||count<1||
      count>static_cast<Steinberg::int64>(SongChain::kCapacity))
      return Steinberg::kInvalidArgument;
   const void* bytes=nullptr;
   Steinberg::uint32 size=0;
   if(attrs->getBinary("Entries",bytes,size)!=Steinberg::kResultOk||
      !bytes||size!=static_cast<Steinberg::uint32>(count*2))
      return Steinberg::kInvalidArgument;
   SongChain candidate;
   if(!candidate.setCount(static_cast<std::size_t>(count)))return Steinberg::kInvalidArgument;
   const auto* entries=static_cast<const std::uint8_t*>(bytes);
   for(std::size_t i=0;i<static_cast<std::size_t>(count);++i)
     if(!candidate.configure(i,entries[2*i],entries[2*i+1]))
       return Steinberg::kInvalidArgument;
   std::lock_guard<std::mutex> guard(kitMutex_);
   kit_.songEnabled=static_cast<std::uint8_t>(enabled);
   kit_.chain=candidate;
   songMailbox_.publish(candidate,enabled!=0);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.MasterVolume")==0) {
   Steinberg::int64 value=-1;
   if(attrs->getInt("Percent",value)!=Steinberg::kResultOk||
      value<0||value>200)return Steinberg::kInvalidArgument;
   masterVolume_.store(static_cast<std::uint8_t>(value),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowRandomVelocity")==0) {
   Steinberg::int64 randomRow=-1,percent=-1;
   if(attrs->getInt("Row",randomRow)!=Steinberg::kResultOk||
      attrs->getInt("Percent",percent)!=Steinberg::kResultOk||
      randomRow<0||randomRow>=static_cast<Steinberg::int64>(kRows)||
      percent<0||percent>100)return Steinberg::kInvalidArgument;
   randomVelocity_[static_cast<std::size_t>(randomRow)].store(
       static_cast<std::uint8_t>(percent),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.Humanize")==0) {
   Steinberg::int64 percent=-1;
   if(attrs->getInt("Percent",percent)!=Steinberg::kResultOk||
      percent<0||percent>100)return Steinberg::kInvalidArgument;
   humanizePercent_.store(static_cast<std::uint8_t>(percent),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.Swing")==0) {
   Steinberg::int64 percent=-1;
   if(attrs->getInt("Percent",percent)!=Steinberg::kResultOk||
      percent<0||percent>50)return Steinberg::kInvalidArgument;
   swingPercent_.store(static_cast<std::uint8_t>(percent),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowOutput")==0) {
   Steinberg::int64 rowIndex=-1,bus=-1;
   if(attrs->getInt("Row",rowIndex)!=Steinberg::kResultOk||
      attrs->getInt("Bus",bus)!=Steinberg::kResultOk||
      rowIndex<0||rowIndex>=static_cast<Steinberg::int64>(kRows)||
      bus<0||bus>=static_cast<Steinberg::int64>(kRows))
      return Steinberg::kInvalidArgument;
   rowOutput_[static_cast<std::size_t>(rowIndex)].store(static_cast<std::uint8_t>(bus),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowChoke")==0) {
   Steinberg::int64 index=-1,group=-1;
   if(attrs->getInt("Row",index)!=Steinberg::kResultOk||
      attrs->getInt("Group",group)!=Steinberg::kResultOk||
      index<0||index>=static_cast<Steinberg::int64>(kRows)||
      group<0||group>4)return Steinberg::kInvalidArgument;
   rowChoke_[static_cast<std::size_t>(index)].store(
      static_cast<std::uint8_t>(group),std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowPitch")==0) {
   Steinberg::int64 rowIndex=-1,semitones=999;
   if(attrs->getInt("Row",rowIndex)!=Steinberg::kResultOk||
      attrs->getInt("Semitones",semitones)!=Steinberg::kResultOk||
      rowIndex<0||rowIndex>=static_cast<Steinberg::int64>(kRows)||
      semitones< -24||semitones>24)return Steinberg::kInvalidArgument;
   const auto idx=static_cast<std::size_t>(rowIndex);
   std::lock_guard<std::mutex> guard(kitMutex_);
   const auto rate=outputRate_.load(std::memory_order_acquire);
   if(!(rate>0.0)||!std::isfinite(rate))return Steinberg::kResultFalse;
   std::unique_ptr<PreparedSample> candidate;
   if(kit_.wav[idx].empty()) {
     try {candidate=PreparedSample::silence(rate);}
     catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
   }else if(PreparedSample::fromWav(
       kit_.wav[idx].data(),kit_.wav[idx].size(),rate,
       static_cast<double>(semitones),1.f,0.f,0,false,candidate,
       kit_.sampleEdits[idx])!=
       PrepareStatus::Ok)return Steinberg::kResultFalse;
   if(!candidate)return Steinberg::kResultFalse;
   for(;;) {auto unused=exchange_.pollReclaim();if(!unused)break;}
   auto pending=exchange_.withdraw(idx);
   (void)pending;
   if(!exchange_.publish(idx,candidate))return Steinberg::kResultFalse;
   kit_.pitch[idx]=static_cast<std::uint8_t>(semitones+24);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowMix")==0) {
   Steinberg::int64 rowIndex=-1,gainValue=-1,panValue=-1;
   if(attrs->getInt("Row",rowIndex)!=Steinberg::kResultOk||
      attrs->getInt("Gain",gainValue)!=Steinberg::kResultOk||
      attrs->getInt("Pan",panValue)!=Steinberg::kResultOk||
      rowIndex<0||rowIndex>=static_cast<Steinberg::int64>(kRows)||
      gainValue<0||gainValue>200||panValue< -100||panValue>100)
      return Steinberg::kInvalidArgument;
   // Gain+Pan arrive in ONE RowMix message. Protect the pair against
   // getState()/setState() snapshots; otherwise one project save can retain
   // the new gain with the previous pan. The audio callback never locks.
   {
     std::lock_guard<std::mutex> guard(kitMutex_);
     rowMix_[static_cast<std::size_t>(rowIndex)].store(
         static_cast<std::uint8_t>(gainValue),
         static_cast<std::uint8_t>(panValue+100));
   }
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.RowGate")==0) {
   Steinberg::int64 isSolo=-1,isOn=-1;
   if(attrs->getInt("Row",row)!=Steinberg::kResultOk||
      attrs->getInt("Solo",isSolo)!=Steinberg::kResultOk||
      attrs->getInt("On",isOn)!=Steinberg::kResultOk||
      row<0||row>=static_cast<Steinberg::int64>(kRows)||
      (isSolo!=0&&isSolo!=1)||(isOn!=0&&isOn!=1))
     return Steinberg::kInvalidArgument;
   auto& mask=isSolo?soloMask_:muteMask_;
   const auto bit=std::uint32_t{1}<<static_cast<unsigned>(row);
   if(isOn)mask.fetch_or(bit,std::memory_order_relaxed);
   else mask.fetch_and(~bit,std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }

 if(std::strcmp(id,"Drumator.VisibleRows")==0) {
   Steinberg::int64 count=0;
   if(attrs->getInt("Count",count)!=Steinberg::kResultOk||
      !RowVisibility::valid(static_cast<std::size_t>(count)))
      return Steinberg::kInvalidArgument;
   // Only the controller hides an empty row. The processor stores the
   // visible count but intentionally retains all 16 DSP slots/steps.
   visibleRows_.store(static_cast<std::uint8_t>(count),
                      std::memory_order_relaxed);
   return Steinberg::kResultOk;
 }
 if(std::strcmp(id,"Drumator.PatternReplace")==0) {
   Steinberg::int64 target=-1;const void* bytes=nullptr;Steinberg::uint32 count=0;
   if(attrs->getInt("Pattern",target)!=Steinberg::kResultOk||
      target<0||target>=static_cast<Steinberg::int64>(kPatterns)||
      attrs->getBinary("Steps",bytes,count)!=Steinberg::kResultOk||
      !bytes||count!=kRows*kSteps)return Steinberg::kInvalidArgument;
   // Legacy PatternReplace sends only Steps. COPY/PASTE/DUPLICATE can
   // also request a 1T/2T duration in the same host control transaction.
   Steinberg::int64 desiredLength=0;
   const bool updateLength=attrs->getInt("Length",desiredLength)==Steinberg::kResultOk;
   if(updateLength&&desiredLength!=16&&desiredLength!=32)
      return Steinberg::kInvalidArgument;
   const auto* values=static_cast<const std::uint8_t*>(bytes);
   for(std::size_t i=0;i<kRows*kSteps;++i)if(values[i]>127)return Steinberg::kInvalidArgument;
   const auto index=static_cast<std::size_t>(target);
   // The exporter holds kitMutex_ before the pattern-bank write lock.
   std::lock_guard<std::mutex> guard(kitMutex_);
   if(!patterns_.replacePattern(index,values,count,
        updateLength?static_cast<std::uint8_t>(desiredLength):0))return Steinberg::kResultFalse;
   if(updateLength) {
      const auto length=static_cast<std::uint8_t>(desiredLength);
      patternLengths_[index].store(length,std::memory_order_release);
      kit_.patternLengths[index]=length;
   }
   return Steinberg::kResultOk;
 }
 if(std::strcmp(msg->getMessageID(),"Drumator.StepEdit")==0) {
   Steinberg::int64 pattern=-1,step=-1,velocity=-1;
   if(attrs->getInt("Pattern",pattern)!=Steinberg::kResultOk||
      attrs->getInt("Row",row)!=Steinberg::kResultOk||
      attrs->getInt("Step",step)!=Steinberg::kResultOk||
      attrs->getInt("Velocity",velocity)!=Steinberg::kResultOk||
      pattern<0||pattern>=static_cast<Steinberg::int64>(kPatterns)||
      row<0||row>=static_cast<Steinberg::int64>(kRows)||
      step<0||step>=static_cast<Steinberg::int64>(kSteps)||
      velocity<0||velocity>127)return Steinberg::kInvalidArgument;
   return patterns_.set(static_cast<std::size_t>(pattern),
                        static_cast<std::size_t>(row),
                        static_cast<std::size_t>(step),
                        static_cast<unsigned>(velocity))
          ? Steinberg::kResultOk : Steinberg::kResultFalse;
 }
 const void* bytes=nullptr;
 Steinberg::uint32 size=0;
 constexpr Steinberg::uint32 kMaxMessageBytes=16u*1024u*1024u;
 if(attrs->getInt("Row",row)!=Steinberg::kResultOk||
    row<0||row>=static_cast<Steinberg::int64>(kRows)||
    attrs->getBinary("WavBytes",bytes,size)!=Steinberg::kResultOk||
    !bytes||size<44||size>kMaxMessageBytes)
    return Steinberg::kInvalidArgument;
 std::string fileName="SAMPLE";
 {
   const void* nameBytes=nullptr;
   Steinberg::uint32 nameLen=0;
   if(attrs->getBinary("SampleName",nameBytes,nameLen)==Steinberg::kResultOk) {
       if(!nameBytes||nameLen==0||nameLen>kMaxKitName)
           return Steinberg::kInvalidArgument;
       try {fileName.assign(static_cast<const char*>(nameBytes),nameLen);}
       catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
   }
   if(fileName.empty()||fileName.find('/')!=std::string::npos||
      fileName.find('\\')!=std::string::npos||
      fileName.find('\0')!=std::string::npos)
       return Steinberg::kInvalidArgument;
 }
 std::vector<std::uint8_t> retained;
 try {retained.assign(static_cast<const std::uint8_t*>(bytes),
                      static_cast<const std::uint8_t*>(bytes)+size);}
 catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
 // Sample preparation and publication must see the same kit generation.
 // In particular, pitch cannot be read concurrently with setState().
 // This mutex is control-thread-only; the realtime process never takes it.
 std::lock_guard<std::mutex> kitGuard(kitMutex_);
 const auto rate=outputRate_.load(std::memory_order_acquire);
 if(!(rate>0.0)||!std::isfinite(rate))return Steinberg::kResultFalse;
 std::unique_ptr<PreparedSample> candidate;
 const auto prepared=PreparedSample::fromWav(
     static_cast<const std::uint8_t*>(bytes),
     static_cast<std::size_t>(size),rate,
     static_cast<double>(static_cast<int>(kit_.pitch[static_cast<std::size_t>(row)])-24),
     1.f,0.f,0,false,candidate,SampleEdit{});
 if(prepared!=PrepareStatus::Ok)return Steinberg::kResultFalse;
 {
    std::size_t total=size;
    for(std::size_t r=0;r<kRows;++r)
      if(r!=static_cast<std::size_t>(row)) {
         if(kit_.wav[r].size()>kMaxKitWavs-total)return Steinberg::kResultFalse;
         total+=kit_.wav[r].size();
      }
    for(;;) {auto unused=exchange_.pollReclaim();if(!unused)break;}
    // A host may open a project or request a sample before the first audio
    // callback has adopted the default empty-row marker. Replace that
    // pending marker on the CONTROL side without touching the audio thread.
    auto previousPending=exchange_.withdraw(static_cast<std::size_t>(row));
    (void)previousPending;
    if(!exchange_.publish(static_cast<std::size_t>(row),candidate))
       return Steinberg::kResultFalse;
    previewPending_.fetch_and(~(std::uint32_t{1}<<static_cast<unsigned>(row)),
                              std::memory_order_acq_rel);
    kit_.wav[static_cast<std::size_t>(row)].swap(retained);
    kit_.sampleEdits[static_cast<std::size_t>(row)]=SampleEdit{};
    kit_.names[static_cast<std::size_t>(row)].swap(fileName);
 }
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::setupProcessing(Steinberg::Vst::ProcessSetup& setup) {
 // The VST3 host may request unusual but valid sample rates. Reject only
 // invalid values or a provably unbounded temporary fixture allocation.
 if(setup.symbolicSampleSize!=Steinberg::Vst::kSample32||
    !std::isfinite(setup.sampleRate)||setup.sampleRate<=0.0||
    setup.maxSamplesPerBlock<0)
   return Steinberg::kInvalidArgument;
 // This runs outside process() and while processing is inactive.
 const auto parent=AudioEffect::setupProcessing(setup);
 if(parent!=Steinberg::kResultTrue)return parent;
 prepared_=false;
 outputRate_.store(0.0,std::memory_order_release);
 voices_.reset(); // old voice buffers are not retained across sample-rate changes
 wasPlaying_=false;
 haveExpectedPpq_=false;
 expectedNextPpq_=0.0;
 phaseOriginStep_=0;
 // A rate change invalidates any prior per-voice interpolation kernel.
 // Preserve retired asset ownership; mute existing rows until reloaded.
 for(std::size_t row=0;row<kRows;++row)voices_.setRow(row,RowSound{});
 // Deliberately silent until a real user sample was actually accepted.
 // No hidden synthesized kick to confuse failed import with working audio.
 if(!voices_.setOutputRate(setup.sampleRate)||
    !masterRamp_.prepare(setup.sampleRate))return Steinberg::kResultFalse;
 masterRamp_.reset(masterVolume_.load(std::memory_order_relaxed)/100.f);
 if(!patternTimeline_.configure(static_cast<std::size_t>(setup.maxSamplesPerBlock)))
   return Steinberg::kOutOfMemory;
 prepared_=true;
 outputRate_.store(setup.sampleRate,std::memory_order_release);
 {
    std::lock_guard<std::mutex> guard(kitMutex_);
    if(!publishRestoredKit(kit_,setup.sampleRate)) {
       outputRate_.store(0.0,std::memory_order_release);
       prepared_=false;return Steinberg::kResultFalse;
    }
 }
 return Steinberg::kResultOk;
}
Steinberg::uint32 PLUGIN_API Processor::getTailSamples() {
 // Internal sequencer can generate sound without MIDI or audio input.
 // The host must continue processing during transport; individual sample
 // duration cannot represent this generator's ongoing output.
 return Steinberg::Vst::kInfiniteTail;
}
Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data) {
 // Validate the host frame count before the restore-gate silence path: a
 // negative count must never be converted to a huge size_t by fill_n.
 if(data.numSamples<0||data.numOutputs<0||
    data.numOutputs>static_cast<Steinberg::int32>(kRows)||
    (data.numOutputs>0&&!data.outputs)||
    (prepared_&&data.numSamples>processSetup.maxSamplesPerBlock))
   return Steinberg::kInvalidArgument;
 // The plugin exposes fixed stereo outputs. Reject impossible host channel
 // counts before the restore-gate silence loop uses numChannels as a bound.
 for(Steinberg::int32 bus=0;bus<data.numOutputs;++bus) {
   const auto channels=data.outputs[bus].numChannels;
   if(channels!=0&&channels!=2)return Steinberg::kInvalidArgument;
 }
 AudioGateEntry currentCall(restoreGate_);
 if(!currentCall.allowed()) {
   // Preserve genuine selection events while audio is gated. Use the same
   // bounded sample timeline and host-over-MIDI priority as ordinary blocks.
   // Do not touch voices or the Song cache being installed by setState().
   if(data.numSamples>0) {
     if(!patternTimeline_.reset(static_cast<std::size_t>(data.numSamples)))
       return Steinberg::kResultFalse;
     if(data.inputParameterChanges)
       for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
         auto* queue=data.inputParameterChanges->getParameterData(i);
         if(!queue||queue->getParameterId()!=kPatternParam)continue;
         for(Steinberg::int32 j=0;j<queue->getPointCount();++j) {
           Steinberg::int32 offset=-1;Steinberg::Vst::ParamValue value=0;
           if(queue->getPoint(j,offset,value)==Steinberg::kResultOk&&std::isfinite(value))
             patternTimeline_.insert(offset,patternFromNormalized(value),true);
         }
       }
     if(data.inputEvents&&!songMailbox_.midiPatternSuppressed())
       for(Steinberg::int32 i=0;i<data.inputEvents->getEventCount();++i) {
         Steinberg::Vst::Event event{};
         if(data.inputEvents->getEvent(i,event)!=Steinberg::kResultOk||
            event.type!=Steinberg::Vst::Event::kNoteOnEvent||
            event.noteOn.pitch<24||event.noteOn.pitch>31||
            !std::isfinite(event.noteOn.velocity)||
            event.noteOn.velocity<=0.f||event.noteOn.velocity>1.f)continue;
         patternTimeline_.insert(event.sampleOffset,
             static_cast<unsigned>(event.noteOn.pitch-24),false);
       }
     Steinberg::Vst::IParamValueQueue* feedback=nullptr;
     for(Steinberg::int32 offset=0;offset<data.numSamples;++offset) {
       const auto point=patternTimeline_.get(static_cast<std::size_t>(offset));
       if(point.source==PatternEventTimeline::kHost)selected_.hostSubmit(point.pattern);
       else if(point.source==PatternEventTimeline::kMidi) {
         selected_.midiSubmit(point.pattern);
         if(data.outputParameterChanges) {
           if(!feedback) {
             Steinberg::int32 index=0;
             feedback=data.outputParameterChanges->addParameterData(kPatternParam,index);
           }
           if(feedback) {
             Steinberg::int32 index=0;
             (void)feedback->addPoint(offset,static_cast<double>(point.pattern)/7.,index);
           }
         }
       }
     }
   }else if(data.inputParameterChanges) {
     // Zero-frame flushes carry offset-zero parameters, no rendered MIDI.
     for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
       auto* q=data.inputParameterChanges->getParameterData(i);
       if(!q||q->getParameterId()!=kPatternParam)continue;
       for(Steinberg::int32 j=0;j<q->getPointCount();++j) {
         Steinberg::int32 offset=0;Steinberg::Vst::ParamValue v=0;
         if(q->getPoint(j,offset,v)!=Steinberg::kResultTrue||
            offset!=0||!std::isfinite(v))continue;
         const auto pattern=patternFromNormalized(v);
         selected_.hostSubmit(pattern);
       }
     }
   }
   if(data.symbolicSampleSize==Steinberg::Vst::kSample32&&
      data.numSamples>0&&data.outputs) {
     for(Steinberg::int32 bus=0;bus<data.numOutputs;++bus) {
       auto& out=data.outputs[bus];
       if(!out.channelBuffers32)continue;
       for(Steinberg::int32 ch=0;ch<out.numChannels;++ch)
         if(out.channelBuffers32[ch])
           std::fill_n(out.channelBuffers32[ch],data.numSamples,0.f);
       out.silenceFlags=3;
     }
   }
   return Steinberg::kResultOk;
 }
 // A valid zero-frame callback can itself be the host's only Stop edge.
 // Observe the explicit transport transition before the parameter-only
 // fast path; otherwise old voices survive a continuous-PPQ restart.
 // Never interpret a missing optional ProcessContext as an explicit Stop.
 if(data.numSamples==0&&data.processContext&&wasPlaying_&&
    (data.processContext->state&Steinberg::Vst::ProcessContext::kPlaying)==0) {
   voices_.reset();
   wasPlaying_=false;
   haveExpectedPpq_=false;
   previewPending_.store(0,std::memory_order_release);
 }
 // Hosts may flush discrete automation without providing any audio buffers.
 if(data.numSamples==0||data.numOutputs==0) {
   // A nonzero block without output buses still carries genuine host and
   // MIDI pattern-selection events. Keep sample-offset/host-tie ordering.
   if(data.numSamples>0&&data.numOutputs==0) {
     // There is no render clock while outputs are absent. Do not resume
     // frozen voices or reuse the previous phase after an outputless gap.
     voices_.reset();
     wasPlaying_=false;
     haveExpectedPpq_=false;
     // An audition requested while there is no audio sink must not fire
     // unexpectedly when the host restores its outputs.
     previewPending_.store(0,std::memory_order_release);
     if(!patternTimeline_.reset(static_cast<std::size_t>(data.numSamples)))
       return Steinberg::kInvalidArgument;
     if(data.inputParameterChanges)
       for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
         auto* q=data.inputParameterChanges->getParameterData(i);
         if(!q||q->getParameterId()!=kPatternParam)continue;
         for(Steinberg::int32 j=0;j<q->getPointCount();++j) {
           Steinberg::int32 offset=-1;
           Steinberg::Vst::ParamValue value=0;
           if(q->getPoint(j,offset,value)==Steinberg::kResultTrue&&std::isfinite(value))
             patternTimeline_.insert(offset,patternFromNormalized(value),true);
         }
       }
     // A concurrently published song edit can make this lock-free
     // snapshot temporarily unavailable. As in the audio-render path,
     // retain the last coherent chain instead of failing process().
     // Refresh the song state every block, even when no MIDI events arrive.
     // Otherwise a transport-only gap retains an outdated song snapshot.
     (void)songMailbox_.snapshot(audioSongChain_,audioSongEnabled_);
     if(data.inputEvents&&!audioSongEnabled_)
       for(Steinberg::int32 i=0;i<data.inputEvents->getEventCount();++i) {
         Steinberg::Vst::Event event{};
         if(data.inputEvents->getEvent(i,event)!=Steinberg::kResultOk||
            event.type!=Steinberg::Vst::Event::kNoteOnEvent||
            event.noteOn.pitch<24||event.noteOn.pitch>31||
            !std::isfinite(event.noteOn.velocity)||
            event.noteOn.velocity<=0.f||event.noteOn.velocity>1.f)continue;
         patternTimeline_.insert(event.sampleOffset,
                                 static_cast<unsigned>(event.noteOn.pitch-24),false);
       }
     Steinberg::Vst::IParamValueQueue* feedback=nullptr;
     for(Steinberg::int32 offset=0;offset<data.numSamples;++offset) {
       const auto point=patternTimeline_.get(static_cast<std::size_t>(offset));
       if(point.source==PatternEventTimeline::kHost)selected_.hostSubmit(point.pattern);
       else if(point.source==PatternEventTimeline::kMidi) {
         selected_.midiSubmit(point.pattern);
         if(data.outputParameterChanges) {
           if(!feedback) {
             Steinberg::int32 index=0;
             feedback=data.outputParameterChanges->addParameterData(kPatternParam,index);
           }
           if(feedback) {
             Steinberg::int32 index=0;
             (void)feedback->addPoint(offset,static_cast<double>(point.pattern)/7.0,index);
           }
         }
       }
     }
     return Steinberg::kResultOk;
   }
   if(data.inputParameterChanges) {
     for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
       auto* queue=data.inputParameterChanges->getParameterData(i);
       if(!queue||queue->getParameterId()!=kPatternParam)continue;
       for(Steinberg::int32 j=0;j<queue->getPointCount();++j) {
         Steinberg::int32 offset=0;
         Steinberg::Vst::ParamValue value=0;
         if(queue->getPoint(j,offset,value)==Steinberg::kResultTrue&&
            offset==0&&std::isfinite(value))
           selected_.hostSubmit(static_cast<std::uint32_t>(patternFromNormalized(value)));
       }
     }
   }
   return Steinberg::kResultOk;
 }
 if(!prepared_||data.symbolicSampleSize!=Steinberg::Vst::kSample32||
    !data.outputs||(data.outputs[0].numChannels!=0&&data.outputs[0].numChannels!=2))
   return Steinberg::kResultFalse;
 auto& output=data.outputs[0];
 // A zero-channel bus has no pointer-array elements, even if the host leaves
 // its pointer union non-null. Never inspect or render those absent channels.
 auto* left=output.numChannels==2&&output.channelBuffers32 ? output.channelBuffers32[0] : nullptr;
 auto* right=output.numChannels==2&&output.channelBuffers32 ? output.channelBuffers32[1] : nullptr;
 // An inactive MAIN bus may legally omit audio buffers while aux buses run.
 const bool mainReady=left&&right;
 if(!mainReady&&((left!=nullptr)!=(right!=nullptr)))return Steinberg::kResultFalse;
 const auto* context=data.processContext;
 const bool playing=context&&
     ((context->state&Steinberg::Vst::ProcessContext::kPlaying)!=0);
 // Never propagate a malformed host clock into expectedNextPpq_: otherwise
 // NaN comparison is always false and the next real locate cannot reset phase.
 const bool validHostClock=context&&
     (context->state&(Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                      Steinberg::Vst::ProcessContext::kTempoValid))==
                     (Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                      Steinberg::Vst::ProcessContext::kTempoValid)&&
     std::isfinite(context->projectTimeMusic)&&
     std::isfinite(context->tempo)&&context->tempo>0.0;
 if(wasPlaying_&&!playing)voices_.reset(); // stop old sequencer tails once
 if(!playing||!validHostClock)haveExpectedPpq_=false;
 wasPlaying_=playing;
 // The first full block after a kit commit starts with no old-kit voices.
 const auto revision=kitRevision_.load(std::memory_order_acquire);
 if(revision!=lastAudioKitRevision_) {
   voices_.reset();
   audioPatternValid_.fill(false); // Never replay an old kit's cached rhythm.
   lastAudioKitRevision_=revision;
 }
 // Adopt only complete prepared assets, without deleting on audio.
 exchange_.audioAdopt(voices_,processSetup.sampleRate);
 triggers_.clear();
 // Collect MIDI A-H and host automation before scheduling. Fixed storage
 // avoids allocations in the audio callback; equal-offset host wins.
 (void)songMailbox_.snapshot(audioSongChain_,audioSongEnabled_);
 if(!patternTimeline_.reset(static_cast<std::size_t>(data.numSamples)))
   return Steinberg::kInvalidArgument;
 auto addPattern=[&](Steinberg::int32 offset,unsigned pattern,bool host) {
   patternTimeline_.insert(offset,pattern,host);
 };
 if(data.inputParameterChanges) {
   for(Steinberg::int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
     auto* q=data.inputParameterChanges->getParameterData(i);
     if(!q||q->getParameterId()!=kPatternParam)continue;
     for(Steinberg::int32 j=0;j<q->getPointCount();++j) {
       Steinberg::int32 offset=-1;Steinberg::Vst::ParamValue v=0;
       if(q->getPoint(j,offset,v)==Steinberg::kResultTrue&&std::isfinite(v))
         addPattern(offset,patternFromNormalized(v),true);
     }
   }
 }
 if(!audioSongEnabled_&&data.inputEvents) {
   for(Steinberg::int32 i=0;i<data.inputEvents->getEventCount();++i) {
     Steinberg::Vst::Event e{};
     if(data.inputEvents->getEvent(i,e)!=Steinberg::kResultOk||
        e.type!=Steinberg::Vst::Event::kNoteOnEvent||
        e.noteOn.pitch<24||e.noteOn.pitch>31||
        !std::isfinite(e.noteOn.velocity)||
        e.noteOn.velocity<=0.f||e.noteOn.velocity>1.f)continue;
     addPattern(e.sampleOffset,static_cast<unsigned>(e.noteOn.pitch-24),false);
   }
 }
 // Preallocated sample timeline replaces the former destructive 256-point overflow.
 Steinberg::Vst::IParamValueQueue* outputPatternQueue=nullptr;
 auto reportMidi=[&](Steinberg::int32 offset,std::uint8_t pattern) {
   if(!data.outputParameterChanges)return;
   if(!outputPatternQueue) {
     Steinberg::int32 index=0;
     outputPatternQueue=data.outputParameterChanges->addParameterData(kPatternParam,index);
     if(!outputPatternQueue)return;
   }
   Steinberg::int32 index=0;
   (void)outputPatternQueue->addPoint(offset,static_cast<double>(pattern)/7.0,index);
 };
 // Process program changes at their actual sample offset. Schedule each
 // constant-pattern interval separately; no allocation in the audio callback.
 if(playing&&validHostClock) {
   const auto initialPpq=context->projectTimeMusic;
   // A new transport run or a locate/loop jump starts at pattern step 1.
   // Normal continuous blocks keep the same origin; switching A-H does not reset it.
   const double rateForClock=processSetup.sampleRate;
   const double expectedTolerance=std::max(1e-7,4.0*context->tempo/(60.0*rateForClock));
   const bool transportJump=haveExpectedPpq_&&
       std::abs(initialPpq-expectedNextPpq_)>expectedTolerance;
   // Some hosts stop invoking process() while stopped. A subsequent backward
   // locate/restart must not carry old sample voices into the new timeline.
   // Ordinary continuous blocks and the first playing block are unaffected.
   if(transportJump)voices_.reset();
   if(!haveExpectedPpq_||transportJump) {
      const double initialStep=std::floor(initialPpq*4.0+1e-8);
      phaseOriginStep_=std::isfinite(initialStep)&&
         initialStep>=static_cast<double>(std::numeric_limits<std::int64_t>::min()/2)&&
         initialStep<=static_cast<double>(std::numeric_limits<std::int64_t>::max()/2)
         ?static_cast<std::int64_t>(initialStep):0;
   }
   expectedNextPpq_=initialPpq+static_cast<double>(data.numSamples)*
       context->tempo/(60.0*rateForClock);
   haveExpectedPpq_=true;
   const auto tempo=context->tempo;
   const auto rate=processSetup.sampleRate;
   const auto swing=swingPercent_.load(std::memory_order_relaxed)/100.0;
   const auto humanize=humanizePercent_.load(std::memory_order_relaxed)/100.0;
   std::uint32_t activePattern=selected_.load(std::memory_order_relaxed);
   std::array<std::uint8_t,kPatterns> lengths{};
   for(std::size_t i=0;i<kPatterns;++i)
     lengths[i]=audioPatternValid_[i]?audioPatternLengthCache_[i]:32;
   std::int32_t cursor=0;
   auto scheduleRange=[&](Steinberg::int32 finish) {
     if(finish<=cursor)return;
     // Read exactly one generation; a concurrently replaced pattern is
     // deferred to a later block rather than rendered in a mixed state.
     std::array<std::uint8_t,kRows*kSteps> consistent{};
     std::uint8_t coherentLength=32;
     if(patterns_.coherentPattern(activePattern,consistent,coherentLength)) {
       audioPatternCache_[activePattern]=consistent;
       audioPatternLengthCache_[activePattern]=coherentLength;
       audioPatternValid_[activePattern]=true;
     }else if(audioPatternValid_[activePattern]) {
       // A controller-side edit must not mute an otherwise valid audio block.
       consistent=audioPatternCache_[activePattern];
     }else if(!audioSongEnabled_) {
       // No previously validated generation exists; never render a partial edit.
       cursor=finish;return;
     }
     if(audioPatternValid_[activePattern])
       lengths[activePattern]=audioPatternLengthCache_[activePattern];
     struct CoherentView {
       const std::array<std::uint8_t,kRows*kSteps>& values;
       std::uint8_t program;
       std::uint8_t selected() const noexcept {return program;}
       std::uint8_t length;
       SongChain::Position mapStep(std::int64_t step,std::int64_t origin) const noexcept {
         const auto period=static_cast<std::int64_t>(length);
         const auto wrapped=((step-origin)%period+period)%period;
         return {program,static_cast<std::uint8_t>(wrapped),0};
       }
       std::uint8_t get(std::size_t p,std::size_t r,std::size_t step) const noexcept {
          return p==program?values[r*kSteps+step]:0;
       }
     } read{consistent,static_cast<std::uint8_t>(activePattern),lengths[activePattern]};
     const Clock clock{initialPpq+static_cast<double>(cursor)*tempo/(60.0*rate),
                       tempo,rate,finish-cursor,true,swing,humanize,phaseOriginStep_};
     std::array<std::array<std::uint8_t,kRows*kSteps>,kPatterns> songPrograms{};
     struct SongRead {
       const std::array<std::array<std::uint8_t,kRows*kSteps>,kPatterns>& values;
       const SongChain& chain;
       const std::array<std::uint8_t,kPatterns>& lengths;
       std::uint8_t selected() const noexcept {return 0;}
       SongChain::Position mapStep(std::int64_t step,std::int64_t origin) const noexcept {
         auto result=chain.at(step,origin);
         result.step=static_cast<std::uint8_t>(result.step%lengths[result.pattern]);
         return result;
       }
       std::uint8_t get(std::size_t program,std::size_t row,std::size_t step) const noexcept {
         return program<kPatterns&&row<kRows&&step<kSteps?
            values[program][row*kSteps+step]:0;
       }
     } songRead{songPrograms,audioSongChain_,lengths};
     if(audioSongEnabled_) {
       for(std::size_t program=0;program<kPatterns;++program){
         std::array<std::uint8_t,kRows*kSteps> coherent{};
         std::uint8_t coherentSongLength=32;
         if(patterns_.coherentPattern(program,coherent,coherentSongLength)){
           audioPatternCache_[program]=coherent;
           audioPatternLengthCache_[program]=coherentSongLength;
           audioPatternValid_[program]=true;
         }
         if(audioPatternValid_[program]) {
           songPrograms[program]=audioPatternCache_[program];
           lengths[program]=audioPatternLengthCache_[program];
         }
       }
     }
     auto emitTrigger=[&](Trigger hit){
       hit.sampleOffset+=static_cast<std::uint32_t>(cursor);
       triggers_.appendStep(hit);
     };
     const auto scheduled=audioSongEnabled_?
           StepScheduler::run(songRead,clock,emitTrigger):
           StepScheduler::run(read,clock,emitTrigger);
     if(scheduled.status!=ScheduleStatus::Ok)triggers_.clear();
     cursor=finish;
   };
   for(Steinberg::int32 offset=0;offset<data.numSamples;++offset) {
     const auto point=patternTimeline_.get(static_cast<std::size_t>(offset));
     if(!point.source)continue;
     scheduleRange(offset);
     activePattern=point.pattern;
      if(point.source==PatternEventTimeline::kHost)selected_.hostSubmit(point.pattern);
      else if(point.source==PatternEventTimeline::kMidi){selected_.midiSubmit(point.pattern);reportMidi(offset,point.pattern);}
   }
   scheduleRange(data.numSamples);

 }else {
   auto active=selected_.load(std::memory_order_relaxed);
   for(Steinberg::int32 offset=0;offset<data.numSamples;++offset) {
     const auto point=patternTimeline_.get(static_cast<std::size_t>(offset));
     if(!point.source)continue;
     active=point.pattern;
      if(point.source==PatternEventTimeline::kHost)selected_.hostSubmit(point.pattern);
      else if(point.source==PatternEventTimeline::kMidi){selected_.midiSubmit(point.pattern);reportMidi(offset,point.pattern);}
   }

 }
 // MIDI one-shot audition also works with stopped DAW transport.
 if(data.inputEvents) {
   for(Steinberg::int32 i=0;i<data.inputEvents->getEventCount();++i) {
     Steinberg::Vst::Event event{};
     if(data.inputEvents->getEvent(i,event)!=Steinberg::kResultOk||
        event.type!=Steinberg::Vst::Event::kNoteOnEvent)continue;
     triggers_.appendNoteOn(event.noteOn.pitch,event.noteOn.velocity,
                            event.sampleOffset,
                            static_cast<std::uint32_t>(data.numSamples));
   }
 }
 // Consume one-shot GUI previews in the audio callback after pattern scheduling.
 // Per-row bit coalescing is bounded and allocation-free; downstream
 // mix/output/choke/mute/solo and master processing remain unchanged.
 const auto pending=previewPending_.exchange(0,std::memory_order_acq_rel);
 for(std::size_t row=0;row<kRows;++row)
   if((pending&(std::uint32_t{1}<<row))!=0)
     (void)triggers_.appendNoteOn(static_cast<std::int32_t>(36+row),1.f,0,
                                   static_cast<std::uint32_t>(data.numSamples));
 for(std::size_t row=0;row<kRows;++row) {
   const auto pair=rowMix_[row].snapshot(); // one coherent UI generation
   if(!voices_.setRowMix(row,AtomicRowMix::gain(pair),AtomicRowMix::pan(pair))||
      !voices_.setRowChoke(row,rowChoke_[row].load(std::memory_order_relaxed))||
       !voices_.setRowOutput(row,rowOutput_[row].load(std::memory_order_relaxed)))
     return Steinberg::kResultFalse;
 }
 triggers_.applyMuteSolo(muteMask_.load(std::memory_order_relaxed),
                         soloMask_.load(std::memory_order_relaxed));
 // Apply realtime-safe per-row random velocity without touching saved steps.
 std::array<std::uint8_t,kRows> variation{};
 for(std::size_t i=0;i<kRows;++i)
   variation[i]=randomVelocity_[i].load(std::memory_order_relaxed);
 const auto validTiming=context&&
     (context->state&(Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                      Steinberg::Vst::ProcessContext::kTempoValid))==
                     (Steinberg::Vst::ProcessContext::kProjectTimeMusicValid|
                      Steinberg::Vst::ProcessContext::kTempoValid)&&
     std::isfinite(context->projectTimeMusic)&&
     std::isfinite(context->tempo)&&context->tempo>0.0;
 triggers_.varyVelocity(variation,validTiming?context->projectTimeMusic:0.0,
                        validTiming?context->tempo:120.0,
                        processSetup.sampleRate);
 triggers_.sort();
 std::array<float*,kRows> stemsL{},stemsR{};
 for(Steinberg::int32 bus=1;bus<data.numOutputs&&
                         bus<static_cast<Steinberg::int32>(kRows);++bus) {
   const auto& stem=data.outputs[bus];
   if((activeStemMask_.load(std::memory_order_relaxed)&
       (std::uint32_t{1}<<static_cast<unsigned>(bus)))&&
      stem.numChannels==2&&stem.channelBuffers32&&
      stem.channelBuffers32[0]&&stem.channelBuffers32[1]) {
     stemsL[static_cast<std::size_t>(bus)]=stem.channelBuffers32[0];
     stemsR[static_cast<std::size_t>(bus)]=stem.channelBuffers32[1];
   }
 }
 const auto rendered=voices_.render(triggers_.data(),triggers_.size(),left,right,
                                   static_cast<std::uint32_t>(data.numSamples),
                                   stemsL.data(),stemsR.data());
 exchange_.audioCollect(voices_);
 if(rendered.status!=RenderStatus::Ok)return Steinberg::kResultFalse;
 const float master=static_cast<float>(masterVolume_.load(std::memory_order_relaxed))/100.f;
 masterRamp_.request(master);
 // MAIN and every active stem share one sample envelope, regardless of bus
 // count. At the usual 100% setting this path avoids all extra channel loops.
 if(!masterRamp_.settledAtUnity()){
   for(Steinberg::int32 frame=0;frame<data.numSamples;++frame){
     const float gain=masterRamp_.next();
     if(gain==1.f)continue;
     for(Steinberg::int32 bus=0;bus<data.numOutputs;++bus){
       const auto& outputBus=data.outputs[bus];
       if(!outputBus.channelBuffers32)continue;
       for(Steinberg::int32 channel=0;channel<outputBus.numChannels;++channel){
         auto* buffer=outputBus.channelBuffers32[channel];
         if(buffer)buffer[frame]*=gain;
       }
     }
   }
 }
 // Conservative: do not claim silence just because a short sound ended within
 // the current block. The host can determine silence independently.
 output.silenceFlags=0;
 for(Steinberg::int32 bus=1;bus<data.numOutputs;++bus)
   data.outputs[bus].silenceFlags=0;
 return Steinberg::kResultOk;
}
// The host/control actor owns serialized sound. The audio thread never reads
// kit_ or its mutex; it only adopts immutable prepared assets at block boundaries.
bool Processor::publishRestoredKit(const KitState& kit,double rate) {
 if(!std::isfinite(rate)||rate<=0.0)return false;
 std::array<std::unique_ptr<PreparedSample>,kRows> fresh{};
 for(std::size_t row=0;row<kRows;++row) {
    if(kit.wav[row].empty()) {
       try {fresh[row]=PreparedSample::silence(rate);}
       catch(const std::bad_alloc&) {return false;}
       if(!fresh[row])return false;
    }else if(PreparedSample::fromWav(
         kit.wav[row].data(),kit.wav[row].size(),rate,
         static_cast<double>(static_cast<int>(kit.pitch[row])-24),1.f,0.f,0,false,
         fresh[row],kit.sampleEdits[row])!=PrepareStatus::Ok)return false;
 }
 for(;;) {auto unused=exchange_.pollReclaim();if(!unused)break;}
 for(std::size_t row=0;row<kRows;++row) {
    auto stale=exchange_.withdraw(row);
    (void)stale;
 }
 for(std::size_t row=0;row<kRows;++row)
    if(!exchange_.publish(row,fresh[row]))return false;
 return true;
}
Steinberg::tresult PLUGIN_API Processor::getState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 std::vector<std::uint8_t> serialized;
 {
   std::lock_guard<std::mutex> guard(kitMutex_);
   kit_.selected=static_cast<std::uint8_t>(selected_.load(std::memory_order_relaxed));
   kit_.visible=visibleRows_.load(std::memory_order_relaxed);
   kit_.muted=static_cast<std::uint16_t>(muteMask_.load(std::memory_order_relaxed));
   kit_.solo=static_cast<std::uint16_t>(soloMask_.load(std::memory_order_relaxed));
   kit_.swing=swingPercent_.load(std::memory_order_relaxed);
   kit_.humanize=humanizePercent_.load(std::memory_order_relaxed);
   kit_.masterVolume=masterVolume_.load(std::memory_order_relaxed);
   for(std::size_t row=0;row<kRows;++row)
      kit_.randomVelocity[row]=randomVelocity_[row].load(std::memory_order_relaxed);
   for(std::size_t i=0;i<kPatterns;++i)
      kit_.patternLengths[i]=patternLengths_[i].load(std::memory_order_relaxed);
   for(std::size_t row=0;row<kRows;++row){kit_.choke[row]=rowChoke_[row].load();kit_.output[row]=rowOutput_[row].load();}
   for(std::size_t row=0;row<kRows;++row) {
     const auto pair=rowMix_[row].snapshot();
     kit_.gain[row]=AtomicRowMix::gainPercent(pair);
     kit_.pan[row]=AtomicRowMix::panEncoded(pair);
   }
   // The regular per-cell getter is safe but not a transaction: a
   // simultaneous PatternReplace could otherwise serialize a torn kit.
   kit_.grid=patterns_.snapshotCoherentForState();
   if(!KitCodec::encode(kit_,serialized))return Steinberg::kResultFalse;
 }
 const auto length=static_cast<Steinberg::TSize>(serialized.size());
 return stream.writeRaw(serialized.data(),length)==length
        ? Steinberg::kResultOk : Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Processor::setState(Steinberg::IBStream* state) {
 if(!state)return Steinberg::kInvalidArgument;
 Steinberg::IBStreamer stream(state,kLittleEndian);
 Steinberg::uint32 magic=0,next=0;
 if(!stream.readInt32u(magic)||!stream.readInt32u(next))
    return Steinberg::kResultFalse;
 KitState candidate;
 if(magic==kKitMagic||magic==kKitMagicV15||magic==kKitMagicV14||magic==kKitMagicV13||magic==kKitMagicV12||magic==kKitMagicV11||magic==kKitMagicV10||magic==kKitMagicV9||magic==kKitMagicV8||magic==kKitMagicV7||magic==kKitMagicV6||magic==kKitMagicV5||magic==kKitMagicV4) {
    if(next<2+kCells+kRows*6+4||next>kMaxKitBytes-8)
       return Steinberg::kResultFalse;
    try {
       std::vector<std::uint8_t> encoded(8+static_cast<std::size_t>(next));
       for(int i=0;i<4;++i) {
           encoded[i]=static_cast<std::uint8_t>(magic>>(8*i));
           encoded[4+i]=static_cast<std::uint8_t>(next>>(8*i));
       }
       if(stream.readRaw(encoded.data()+8,next)!=static_cast<Steinberg::TSize>(next)||
          !KitCodec::decode(encoded.data(),encoded.size(),candidate))
          return Steinberg::kResultFalse;
    }catch(const std::bad_alloc&) {return Steinberg::kOutOfMemory;}
 }else {
    if(next>=kPatterns)return Steinberg::kResultFalse;
    candidate.selected=static_cast<std::uint8_t>(next);
    candidate.visible=static_cast<std::uint8_t>(kRows);
    if(magic==kStateMagic) {
       AtomicPatternBank defaults;
       for(std::size_t p=0;p<kPatterns;++p)
        for(std::size_t r=0;r<kRows;++r)
         for(std::size_t s=0;s<kSteps;++s)
          candidate.grid[AtomicPatternBank::index(p,r,s)]=defaults.get(p,r,s);
    }else if(magic==kStateMagicV2||magic==kStateMagicV3Local) {
       if(magic==kStateMagicV3Local &&
          (!stream.readInt8u(candidate.visible)||
            !RowVisibility::valid(candidate.visible)))
          return Steinberg::kResultFalse;
       for(auto& cell:candidate.grid)
          if(!stream.readInt8u(cell)||cell>127)
             return Steinberg::kResultFalse;
    }else return Steinberg::kResultFalse;
 }
 // Valid imported kits may contain solo flags on rows that are hidden.
 // They are not user-operable; drop only those flags, preserving audio assets.
 candidate.solo=RowVisibility::visibleSolo(candidate.solo,candidate.visible);
 const auto rate=outputRate_.load(std::memory_order_acquire);
 {
    std::lock_guard<std::mutex> guard(kitMutex_);
    const auto selectionAtStart=selected_.snapshot();
    ControlRestoreEntry restoreGate(restoreGate_);
    if(rate>0.0&&!publishRestoredKit(candidate,rate))
       return Steinberg::kResultFalse;
    previewPending_.store(0,std::memory_order_release);
    kit_=std::move(candidate);
    for(std::size_t i=0;i<kPatterns;++i)
      patternLengths_[i].store(kit_.patternLengths[i],std::memory_order_relaxed);
    songMailbox_.publish(kit_.chain,kit_.songEnabled!=0);
    // Seed the fallback with THIS kit before audio can re-enter. If a later
    // SongConfig publication overlaps the first block, its rejected snapshot
    // must not fall back to the previous kit's chain/mode. Gate-denied audio
    // handles selections without reading these non-atomic cache fields.
    audioSongChain_=kit_.chain;
    audioSongEnabled_=kit_.songEnabled!=0;
    // Publish restored rhythms per complete pattern generation. A live
    // scheduler must never observe a half-restored pattern.
    for(std::size_t p=0;p<kPatterns;++p)
      if(!patterns_.replacePattern(p,
             kit_.grid.data()+p*kRows*kSteps,kRows*kSteps,kit_.patternLengths[p]))
        return Steinberg::kResultFalse;
    selected_.restoreIfUnchanged(selectionAtStart,kit_.selected);
    patterns_.select(kit_.selected);
    visibleRows_.store(kit_.visible,std::memory_order_relaxed);
    muteMask_.store(kit_.muted,std::memory_order_relaxed);
    soloMask_.store(kit_.solo,std::memory_order_relaxed);
    swingPercent_.store(kit_.swing,std::memory_order_relaxed);
    humanizePercent_.store(kit_.humanize,std::memory_order_relaxed);
    masterVolume_.store(kit_.masterVolume,std::memory_order_relaxed);
    // Host project recall is immediate; the restore gate has quiesced audio.
    masterRamp_.reset(static_cast<float>(kit_.masterVolume)/100.f);
    for(std::size_t row=0;row<kRows;++row)
       randomVelocity_[row].store(kit_.randomVelocity[row],std::memory_order_relaxed);
    for(std::size_t row=0;row<kRows;++row){rowChoke_[row].store(kit_.choke[row]);rowOutput_[row].store(kit_.output[row]);}
    for(std::size_t row=0;row<kRows;++row)
      rowMix_[row].store(kit_.gain[row],kit_.pan[row]);
    // Kit selection CAS above cannot overwrite newer host automation.
    kitRevision_.fetch_add(1,std::memory_order_release);
 }
 return Steinberg::kResultOk;
}
}
