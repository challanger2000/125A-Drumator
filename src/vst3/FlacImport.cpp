#include "core/FlacImport.h"
#include "core/WavPcm16Decoder.h"
#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <vector>

namespace drumator {
bool flacToWav(const std::uint8_t* data,std::size_t size,
               std::vector<std::uint8_t>& out) noexcept {
 if(!data||size<16||size>16u*1024u*1024u)return false;
 unsigned int channels=0,rate=0;
 drflac_uint64 frames=0;
 float* pcm=drflac_open_memory_and_read_pcm_frames_f32(
    data,size,&channels,&rate,&frames,nullptr);
 if(!pcm)return false;
 const auto release=[&](){drflac_free(pcm,nullptr);};
 if((channels!=1&&channels!=2)||rate==0||frames==0||
    frames>WavPcm16Decoder::kMaxFrames||
    frames>(16u*1024u*1024u-44u)/(channels*4u)) {
   release();return false;
 }
 try {
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
     const float x=pcm[i];
     if(!std::isfinite(x)||std::fabs(x)>16.f){release();return false;}
     std::uint32_t raw=0;std::memcpy(&raw,&x,4);u32(raw);
   }
   release();
   out.swap(wav);
   return true;
 } catch(const std::bad_alloc&) {release();return false;}
}
}
