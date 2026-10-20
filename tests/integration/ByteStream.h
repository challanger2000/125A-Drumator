#pragma once
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
namespace drumator::qa {
// Read-only host stream: no hidden allocations during controller fault injection.
class ByteStream final:public Steinberg::U::Implements<Steinberg::U::Directly<Steinberg::IBStream>> {
 const std::uint8_t* data_;std::size_t size_,cursor_{0};
public:
 ByteStream(const void* data,std::size_t size):data_(static_cast<const std::uint8_t*>(data)),size_(size){}
 Steinberg::tresult PLUGIN_API read(void* out,Steinberg::int32 n,Steinberg::int32* actual) override {
   if(n<0||!out)return Steinberg::kInvalidArgument;
   const auto count=std::min(static_cast<std::size_t>(n),size_-cursor_);
   if(count)std::memcpy(out,data_+cursor_,count);cursor_+=count;
   if(actual)*actual=static_cast<Steinberg::int32>(count);return Steinberg::kResultOk;
 }
 Steinberg::tresult PLUGIN_API write(void*,Steinberg::int32,Steinberg::int32*) override{return Steinberg::kNotImplemented;}
 Steinberg::tresult PLUGIN_API seek(Steinberg::int64 offset,Steinberg::int32 mode,Steinberg::int64* result) override {
   const auto base=mode==kIBSeekSet?0:mode==kIBSeekCur?cursor_:size_;
   if(base>static_cast<std::size_t>(std::numeric_limits<Steinberg::int64>::max()))return Steinberg::kInvalidArgument;
   const auto next=static_cast<Steinberg::int64>(base)+offset;
   if(next<0||static_cast<std::uint64_t>(next)>size_)return Steinberg::kInvalidArgument;
   cursor_=static_cast<std::size_t>(next);if(result)*result=next;return Steinberg::kResultOk;
 }
 Steinberg::tresult PLUGIN_API tell(Steinberg::int64* pos) override {
   if(!pos)return Steinberg::kInvalidArgument;*pos=static_cast<Steinberg::int64>(cursor_);return Steinberg::kResultOk;
 }
};
}
