// 125A Drumator - strict RIFF/WAVE PCM16 decoder.
// Use ONLY on a control/worker thread; returned buffers own decoded audio.
// Entire input is already resident in memory. No filesystem access here.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <utility>
#include <vector>

namespace drumator {
struct DecodedWav {
  std::vector<float> left, right; // right empty for mono
  std::uint32_t sampleRate=0;
  std::uint16_t channels=0;
};
enum class WavError {
  Ok, InvalidContainer, InvalidFormat, UnsupportedFormat,
  Truncated, TooLarge, AllocationFailure
};
class WavPcm16Decoder {
  static std::uint16_t u16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0])|
           static_cast<std::uint16_t>(p[1])<<8;
  }
  static std::uint32_t u32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0])|
          (static_cast<std::uint32_t>(p[1])<<8)|
          (static_cast<std::uint32_t>(p[2])<<16)|
          (static_cast<std::uint32_t>(p[3])<<24);
  }
  static bool id(const std::uint8_t* p,const char* s) noexcept {
    return std::memcmp(p,s,4)==0;
  }
public:
  // Guardrail for untrusted files. Not an audio-thread function.
  static constexpr std::size_t kMaxFrames=20'000'000;
  static WavError decode(const std::uint8_t* bytes,std::size_t size,
                         DecodedWav& output) {
    if(!bytes||size<12)return WavError::InvalidContainer;
    if(!id(bytes,"RIFF")||!id(bytes+8,"WAVE"))
      return WavError::UnsupportedFormat;
    const std::size_t riffSize=u32(bytes+4);
    if(riffSize<4||riffSize>size-8)return WavError::Truncated;
    const std::size_t end=riffSize+8;
    std::size_t pos=12;
    bool hasFmt=false,hasData=false;
    std::uint16_t channels=0,blockAlign=0;
    std::uint32_t sampleRate=0,byteRate=0;
    std::size_t dataStart=0,dataSize=0;
    while(pos+8<=end) {
      const std::size_t length=u32(bytes+pos+4);
      const std::size_t start=pos+8;
      if(length>end-start)return WavError::Truncated;
      if(id(bytes+pos,"fmt ")) {
        if(hasFmt||length<16)return WavError::InvalidFormat;
        hasFmt=true;
        const auto format=u16(bytes+start);
        channels=u16(bytes+start+2);
        sampleRate=u32(bytes+start+4);
        byteRate=u32(bytes+start+8);
        blockAlign=u16(bytes+start+12);
        const auto bits=u16(bytes+start+14);
        // PCM WAVE format 1, signed 16-bit. Extensible/float not claimed.
        if(format!=1||bits!=16||!(channels==1||channels==2))
          return WavError::UnsupportedFormat;
        if(sampleRate==0||blockAlign!=channels*2u||
           static_cast<std::uint64_t>(byteRate)!=
             static_cast<std::uint64_t>(sampleRate)*blockAlign)
          return WavError::InvalidFormat;
      } else if(id(bytes+pos,"data")) {
        if(hasData)return WavError::InvalidFormat;
        hasData=true;dataStart=start;dataSize=length;
      }
      const auto padded=length+(length&1u);
      if(padded>end-start)return WavError::Truncated;
      pos=start+padded;
    }
    if(pos!=end||!hasFmt||!hasData)return WavError::InvalidFormat;
    if(dataSize%blockAlign)return WavError::InvalidFormat;
    const std::size_t frames=dataSize/blockAlign;
    if(frames>kMaxFrames)return WavError::TooLarge;
    DecodedWav candidate;
    candidate.channels=channels;candidate.sampleRate=sampleRate;
    try {
      candidate.left.resize(frames);
      if(channels==2)candidate.right.resize(frames);
    } catch(const std::bad_alloc&) {
      return WavError::AllocationFailure;
    }
    for(std::size_t i=0;i<frames;++i) {
      const auto* frame=bytes+dataStart+i*blockAlign;
      const auto l=static_cast<std::int16_t>(u16(frame));
      candidate.left[i]=static_cast<float>(l)/32768.f;
      if(channels==2) {
        const auto r=static_cast<std::int16_t>(u16(frame+2));
        candidate.right[i]=static_cast<float>(r)/32768.f;
      }
    }
    output=std::move(candidate);
    return WavError::Ok;
  }
};
} // namespace drumator
