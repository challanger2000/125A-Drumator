#include "core/FlacImport.h"
#include "core/WavPcm16Decoder.h"
// A warning in pinned upstream dr_flac 6575 is not actionable in our wrapper.
// Suppress precisely its MSVC size_t narrowing warning, not project warnings.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4267)
#endif
#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <vector>

namespace drumator {
bool flacToWav(const std::uint8_t* data,std::size_t size,
               std::vector<std::uint8_t>& out) noexcept {
 if(!data||size<16||size>16u*1024u*1024u)return false;
 // Open metadata first; never let the decoder allocate a full untrusted stream.
 drflac* decoder=drflac_open_memory(data,size,nullptr);
 if(!decoder)return false;
 const auto close=[&](){if(decoder){drflac_close(decoder);decoder=nullptr;}};
 const unsigned int channels=decoder->channels,rate=decoder->sampleRate;
 constexpr std::size_t kMaxWavBytes=16u*1024u*1024u;
 if((channels!=1&&channels!=2)||rate==0||
    rate>std::numeric_limits<std::uint32_t>::max()/(channels*4u)) {
   close();return false;
 }
 const std::size_t maxFrames=std::min<std::size_t>(
     WavPcm16Decoder::kMaxFrames,(kMaxWavBytes-44u)/(channels*4u));
 if(decoder->totalPCMFrameCount>maxFrames) {
   close();return false;
 }
 try {
   std::vector<float> samples;
   if(decoder->totalPCMFrameCount)
     samples.reserve(static_cast<std::size_t>(decoder->totalPCMFrameCount)*channels);
   std::array<float,4096*2> chunk{};
   const auto declaredFrames=decoder->totalPCMFrameCount;
   std::size_t framesRead=0;
   while(true) {
     const auto count=drflac_read_pcm_frames_f32(decoder,4096,chunk.data());
     if(count==0)break;
     if(count>maxFrames-framesRead) {close();return false;}
     framesRead+=static_cast<std::size_t>(count);
     samples.insert(samples.end(),chunk.data(),
                    chunk.data()+static_cast<std::size_t>(count)*channels);
   }
   close();
   if(framesRead==0||(declaredFrames&&framesRead!=declaredFrames))return false;
   const std::size_t frames=framesRead;

   const auto payload=static_cast<std::size_t>(frames)*channels*4u;
   std::vector<std::uint8_t> wav;
   wav.reserve(44+payload);
   auto tag=[&](const char* s){for(int i=0;i<4;++i)wav.push_back(static_cast<std::uint8_t>(s[i]));};
   auto u16=[&](std::uint16_t v){wav.push_back(static_cast<std::uint8_t>(v));
                                 wav.push_back(static_cast<std::uint8_t>(v>>8));};
   auto u32=[&](std::uint32_t v){for(int i=0;i<4;++i)
                                 wav.push_back(static_cast<std::uint8_t>(v>>(8*i)));};
   tag("RIFF");u32(static_cast<std::uint32_t>(36+payload));tag("WAVE");
   tag("fmt ");u32(16);u16(3);u16(static_cast<std::uint16_t>(channels));
   u32(rate);u32(rate*channels*4u);u16(static_cast<std::uint16_t>(channels*4));u16(32);
   tag("data");u32(static_cast<std::uint32_t>(payload));
   for(std::size_t i=0;i<static_cast<std::size_t>(frames)*channels;++i) {
     const float x=samples[i];
     if(!std::isfinite(x)||std::fabs(x)>16.f)return false;
     std::uint32_t raw=0;std::memcpy(&raw,&x,4);u32(raw);
   }
   out.swap(wav);
   return true;
 } catch(const std::bad_alloc&) {close();return false;}
}
}
