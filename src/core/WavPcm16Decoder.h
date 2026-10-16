// 125A Drumator - strict RIFF/WAVE PCM and IEEE float decoder.
// Off-audio-thread only. Input and output are separate; failure does not
// mutate the previous valid output. No external codec or license dependency.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <utility>
#include <vector>

namespace drumator {
struct DecodedWav {
 std::vector<float> left,right;
 std::uint32_t sampleRate=0;
 std::uint16_t channels=0;
};
enum class WavError {Ok,InvalidContainer,InvalidFormat,UnsupportedFormat,
                     Truncated,TooLarge,AllocationFailure};
class WavPcm16Decoder {
 static std::uint16_t u16(const std::uint8_t* p) noexcept {
   return static_cast<std::uint16_t>(p[0])|
     static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1])<<8);
 }
 static std::uint32_t u32(const std::uint8_t* p) noexcept {
   return static_cast<std::uint32_t>(p[0])|
     (static_cast<std::uint32_t>(p[1])<<8)|
     (static_cast<std::uint32_t>(p[2])<<16)|
     (static_cast<std::uint32_t>(p[3])<<24);
 }
 static bool id(const std::uint8_t* p,const char* str) noexcept {
    return std::memcmp(p,str,4)==0;
 }
 static std::int64_t signedPcm(const std::uint8_t* p,std::uint16_t bits) noexcept {
   if(bits==8)return static_cast<std::int64_t>(p[0])-128;
   std::uint64_t value=0;
   for(std::uint16_t i=0;i<bits/8;++i)
      value|=static_cast<std::uint64_t>(p[i])<<(i*8);
   const auto sign=std::uint64_t{1}<<(bits-1);
   return value>=sign
      ?static_cast<std::int64_t>(value)-(std::int64_t{1}<<bits)
      :static_cast<std::int64_t>(value);
 }
 static bool extensible(const std::uint8_t* p,std::size_t length,
                        std::uint16_t& type,std::uint16_t bits) noexcept {
   if(length<40||u16(p+16)<22)return false;
   const auto valid=u16(p+18);
   if(valid!=bits)return false; // do not misinterpret packed-depth padding
   // WAVE GUID last bytes: 80 00 00 AA 00 38 9B 71
   const auto* guid=p+24;
   if(guid[0]!=1&&guid[0]!=3)return false;
   if(guid[1]!=0||guid[2]!=0||guid[3]!=0)return false;
   // remaining GUID bytes for 00000001/3-0000-0010-8000-00AA00389B71
   static constexpr std::uint8_t rest[12]={
      0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71
   };
   if(std::memcmp(guid+4,rest,12)!=0)return false;
   type=guid[0];
   return true;
 }
public:
 static constexpr std::size_t kMaxFrames=20'000'000;
 static WavError decode(const std::uint8_t* bytes,std::size_t size,
                        DecodedWav& output) {
   if(!bytes||size<12)return WavError::InvalidContainer;
   if(!id(bytes,"RIFF")||!id(bytes+8,"WAVE"))
      return WavError::UnsupportedFormat; // RF64/RIFX not claimed
   const std::size_t declared=u32(bytes+4);
   if(declared<4||declared>size-8)return WavError::Truncated;
   const std::size_t end=declared+8;
   std::size_t cursor=12,dataPos=0,dataLen=0;
   std::uint16_t format=0,channels=0,bits=0,align=0;
   std::uint32_t rate=0,byteRate=0;
   bool hasFmt=false,hasData=false;
   while(cursor+8<=end) {
      const auto len=static_cast<std::size_t>(u32(bytes+cursor+4));
      const auto begin=cursor+8;
      if(len>end-begin)return WavError::Truncated;
      if(id(bytes+cursor,"fmt ")) {
         if(hasFmt||len<16)return WavError::InvalidFormat;
         hasFmt=true;
         const auto* fmt=bytes+begin;
         format=u16(fmt);channels=u16(fmt+2);rate=u32(fmt+4);
         byteRate=u32(fmt+8);align=u16(fmt+12);bits=u16(fmt+14);
         if(format==0xfffe) {
            if(!extensible(fmt,len,format,bits))
               return WavError::UnsupportedFormat;
         }
         if(!(channels==1||channels==2)||
            !((format==1&&(bits==8||bits==16||bits==24||bits==32))||
              (format==3&&bits==32)))
            return WavError::UnsupportedFormat;
         if(rate==0||align!=channels*(bits/8)||
            static_cast<std::uint64_t>(byteRate)!=
            static_cast<std::uint64_t>(rate)*align)
            return WavError::InvalidFormat;
      } else if(id(bytes+cursor,"data")) {
         if(hasData)return WavError::InvalidFormat;
         hasData=true;dataPos=begin;dataLen=len;
      }
      if(len+(len&1u)>end-begin)return WavError::Truncated;
      cursor=begin+len+(len&1u);
   }
   if(cursor!=end||!hasFmt||!hasData)return WavError::InvalidFormat;
   if(dataLen%align)return WavError::InvalidFormat;
   const auto frames=dataLen/align;
   if(frames==0||frames>kMaxFrames)return WavError::TooLarge;
   DecodedWav candidate;
   candidate.sampleRate=rate;candidate.channels=channels;
   try {
     candidate.left.resize(frames);
     if(channels==2)candidate.right.resize(frames);
   }catch(const std::bad_alloc&) {return WavError::AllocationFailure;}
   const auto read=[=](const std::uint8_t* data,float& value) noexcept {
      if(format==3) {
         const auto raw=u32(data);
         std::memcpy(&value,&raw,sizeof(float));
         return std::isfinite(value) && std::fabs(value)<=16.f;
      }
      const auto integral=signedPcm(data,bits);
      value=static_cast<float>(static_cast<double>(integral)/
                                 static_cast<double>(std::uint64_t{1}<<(bits-1)));
      return true;
   };
   for(std::size_t i=0;i<frames;++i) {
      const auto* frame=bytes+dataPos+i*align;
      if(!read(frame,candidate.left[i]))return WavError::InvalidFormat;
      if(channels==2&&!read(frame+(bits/8),candidate.right[i]))
         return WavError::InvalidFormat;
   }
   output=std::move(candidate);
   return WavError::Ok;
 }
};
}
