// 125A Drumator DMR4 portable embedded-kit project state.
// All operations are control/host-state thread only; NEVER call from process().
#pragma once
#include "SequencerCore.h"
#include "VisibleRows.h"
#include "WavPcm16Decoder.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>
namespace drumator {
constexpr std::uint32_t kKitMagic=0x34524d44u; // "DMR4", little endian
constexpr std::size_t kMaxSingleWav=16u*1024u*1024u;
constexpr std::size_t kMaxKitWavs=64u*1024u*1024u;
constexpr std::size_t kMaxKitName=512;
constexpr std::size_t kMaxKitBytes=kMaxKitWavs+kRows*(kMaxKitName+6u)+kCells+16u;
struct KitState {
 std::uint8_t selected{0};
 std::uint8_t visible{kDefaultVisibleRows};
 std::array<std::uint8_t,kCells> grid{};
 std::array<std::string,kRows> names{};
 std::array<std::vector<std::uint8_t>,kRows> wav{};
};
struct KitCodec {
 static std::uint16_t u16(const std::uint8_t* p) noexcept {
   return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1])<<8));
 }
 static std::uint32_t u32(const std::uint8_t* p) noexcept {
   return static_cast<std::uint32_t>(p[0]) |
      (static_cast<std::uint32_t>(p[1])<<8) |
      (static_cast<std::uint32_t>(p[2])<<16) |
      (static_cast<std::uint32_t>(p[3])<<24);
 }
 static void put16(std::vector<std::uint8_t>& out,std::uint16_t n) {
   out.push_back(static_cast<std::uint8_t>(n));
   out.push_back(static_cast<std::uint8_t>(n>>8));
 }
 static void put32(std::vector<std::uint8_t>& out,std::uint32_t n) {
   for(int i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(n>>(8*i)));
 }
 static std::uint32_t hash(const std::uint8_t* p,std::size_t n) noexcept {
   std::uint32_t value=2166136261u;
   for(std::size_t i=0;i<n;++i) {
      value^=p[i];
      value*=16777619u;
   }
   return value;
 }
 static bool valid(const KitState& src,bool validateWav) {
   if(src.selected>=kPatterns||!RowVisibility::valid(src.visible))return false;
   for(const auto v:src.grid)if(v>127)return false;
   std::size_t sum=0;
   for(std::size_t r=0;r<kRows;++r) {
      const auto& bytes=src.wav[r];
      const auto& name=src.names[r];
      if(name.size()>kMaxKitName||bytes.size()>kMaxSingleWav ||
         bytes.size()>kMaxKitWavs-sum ||
         (!bytes.empty()&&name.empty()))return false;
      if(name.find('\0')!=std::string::npos ||
         name.find('/')!=std::string::npos ||
         name.find('\\')!=std::string::npos)return false;
      sum+=bytes.size();
      if(validateWav&&!bytes.empty()) {
          DecodedWav check;
          if(WavPcm16Decoder::decode(bytes.data(),bytes.size(),check)!=WavError::Ok)
             return false;
      }
   }
   return true;
 }
 static bool encode(const KitState& state,std::vector<std::uint8_t>& out) {
   if(!valid(state,false))return false;
   try {
     std::vector<std::uint8_t> value;
     std::size_t size=8+2+kCells+4; // header, selected/count, cells, checksum
     for(std::size_t r=0;r<kRows;++r)
         size+=6+state.names[r].size()+state.wav[r].size();
     if(size>kMaxKitBytes||size-8>std::numeric_limits<std::uint32_t>::max())
         return false;
     value.reserve(size);
     put32(value,kKitMagic);
     put32(value,static_cast<std::uint32_t>(size-8));
     value.push_back(state.selected);
     value.push_back(state.visible);
     value.insert(value.end(),state.grid.begin(),state.grid.end());
     for(std::size_t r=0;r<kRows;++r) {
        const auto& name=state.names[r];
        const auto& audio=state.wav[r];
        put16(value,static_cast<std::uint16_t>(name.size()));
        put32(value,static_cast<std::uint32_t>(audio.size()));
        value.insert(value.end(),name.begin(),name.end());
        value.insert(value.end(),audio.begin(),audio.end());
     }
     put32(value,hash(value.data()+8,value.size()-8));
     if(value.size()!=size)return false;
     out=std::move(value);
     return true;
   }catch(const std::bad_alloc&) {return false;}
 }
 static bool decode(const std::uint8_t* data,std::size_t size,KitState& out,
                    bool validateWav=true) {
   if(!data||size<8+2+kCells+6*kRows+4||size>kMaxKitBytes ||
      u32(data)!=kKitMagic||u32(data+4)!=size-8)return false;
   if(u32(data+size-4)!=hash(data+8,size-12))return false;
   try {
      KitState candidate;
      std::size_t pos=8;
      candidate.selected=data[pos++];
      candidate.visible=data[pos++];
      if(candidate.selected>=kPatterns||!RowVisibility::valid(candidate.visible))
          return false;
      std::memcpy(candidate.grid.data(),data+pos,kCells);
      pos+=kCells;
      std::size_t bytesTotal=0;
      const auto end=size-4;
      for(std::size_t r=0;r<kRows;++r) {
          if(end-pos<6)return false;
          const auto nameLen=static_cast<std::size_t>(u16(data+pos));
          const auto audioLen=static_cast<std::size_t>(u32(data+pos+2));
          pos+=6;
          if(nameLen>kMaxKitName||audioLen>kMaxSingleWav||
             audioLen>kMaxKitWavs-bytesTotal||
             nameLen>end-pos||audioLen>end-pos-nameLen)return false;
          candidate.names[r].assign(reinterpret_cast<const char*>(data+pos),nameLen);
          pos+=nameLen;
          candidate.wav[r].assign(data+pos,data+pos+audioLen);
          pos+=audioLen;
          bytesTotal+=audioLen;
      }
      if(pos!=end||!valid(candidate,validateWav))return false;
      out=std::move(candidate);
      return true;
   }catch(const std::bad_alloc&) {return false;}
 }
};
}
