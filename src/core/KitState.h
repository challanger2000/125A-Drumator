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
#include "SongChain.h"
namespace drumator {
constexpr std::uint32_t kKitMagicV4=0x34524d44u;
constexpr std::uint32_t kKitMagicV5=0x35524d44u;
constexpr std::uint32_t kKitMagicV6=0x36524d44u;
constexpr std::uint32_t kKitMagicV7=0x37524d44u;
constexpr std::uint32_t kKitMagicV8=0x38524d44u;
constexpr std::uint32_t kKitMagicV9=0x39524d44u;
constexpr std::uint32_t kKitMagicV10=0x30314d44u; // DM10
constexpr std::uint32_t kKitMagicV11=0x31314d44u; // DM11
constexpr std::uint32_t kKitMagicV12=0x32314d44u; // DM12
constexpr std::uint32_t kKitMagic=0x33314d44u; // DM13
constexpr std::size_t kMaxSingleWav=16u*1024u*1024u;
constexpr std::size_t kMaxKitWavs=64u*1024u*1024u;
constexpr std::size_t kMaxKitName=512;
constexpr std::size_t kMaxKitBytes=kMaxKitWavs+kRows*(kMaxKitName+6u)+kCells+20u+5u*kRows+2u+130u+kRows+1u;
struct KitState {
 std::uint8_t selected{0};
 std::uint8_t visible{kDefaultVisibleRows};
 std::uint16_t muted{0};
 std::uint16_t solo{0};
 std::uint8_t swing{0};
 std::uint8_t humanize{0};
 std::uint8_t songEnabled{0};
 std::uint8_t masterVolume{100};
 std::array<std::uint8_t,kRows> randomVelocity{};
 SongChain chain{};
 std::array<std::uint8_t,kRows> gain{};
 std::array<std::uint8_t,kRows> pan{};
 std::array<std::uint8_t,kRows> pitch{};
 std::array<std::uint8_t,kRows> choke{};
 std::array<std::uint8_t,kRows> output{};
 KitState(){gain.fill(100);pan.fill(100);pitch.fill(24);
 for(std::size_t row=0;row<kRows;++row)output[row]=static_cast<std::uint8_t>(row);}

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
   if(src.swing>50||src.humanize>100||src.masterVolume>200)return false;
    for(const auto v:src.randomVelocity)if(v>100)return false;
   if(src.songEnabled>1||src.chain.count()<1||src.chain.count()>SongChain::kCapacity)return false;
   for(std::size_t i=0;i<src.chain.count();++i){const auto e=src.chain.entry(i);if(e.pattern>=kPatterns||!SongChain::validLength(e.steps))return false;}
   for(const auto v:src.grid)if(v>127)return false;
   for(auto x:src.gain)if(x>200)return false;
   for(auto x:src.pan)if(x>200)return false;
   for(auto x:src.pitch)if(x>48)return false;
   for(auto x:src.choke)if(x>4)return false;
   for(auto x:src.output)if(x>=kRows)return false;
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
     std::size_t size=8+2+4+5*kRows+2+130+kRows+1+kCells+4; // header, selected/count, cells, checksum
     for(std::size_t r=0;r<kRows;++r)
         size+=6+state.names[r].size()+state.wav[r].size();
     if(size>kMaxKitBytes||size-8>std::numeric_limits<std::uint32_t>::max())
         return false;
     value.reserve(size);
     put32(value,kKitMagic);
     put32(value,static_cast<std::uint32_t>(size-8));
     value.push_back(state.selected);
     value.push_back(state.visible);
     put16(value,state.muted);put16(value,state.solo);
     value.insert(value.end(),state.gain.begin(),state.gain.end());
     value.insert(value.end(),state.pan.begin(),state.pan.end());
     value.insert(value.end(),state.pitch.begin(),state.pitch.end());
     value.insert(value.end(),state.choke.begin(),state.choke.end());
     value.insert(value.end(),state.output.begin(),state.output.end());
     value.push_back(state.swing);
     value.push_back(state.humanize);
     value.push_back(state.songEnabled);
     value.push_back(static_cast<std::uint8_t>(state.chain.count()));
     for(std::size_t i=0;i<SongChain::kCapacity;++i){const auto e=state.chain.entry(i);value.push_back(e.pattern);value.push_back(e.steps);}
     value.push_back(state.masterVolume);
     value.insert(value.end(),state.randomVelocity.begin(),state.randomVelocity.end());
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
      (u32(data)!=kKitMagic&&u32(data)!=kKitMagicV12&&u32(data)!=kKitMagicV11&&u32(data)!=kKitMagicV10&&u32(data)!=kKitMagicV9&&u32(data)!=kKitMagicV8&&u32(data)!=kKitMagicV7&&u32(data)!=kKitMagicV6&&u32(data)!=kKitMagicV5&&u32(data)!=kKitMagicV4)||
      u32(data+4)!=size-8)return false;
   const bool v13=u32(data)==kKitMagic;
   const bool v12=v13||u32(data)==kKitMagicV12;
   const bool v11=v12||u32(data)==kKitMagicV11;
   const bool v10=v11||u32(data)==kKitMagicV10;
   const bool v9=v10||u32(data)==kKitMagicV9;
   const bool v8=v9||u32(data)==kKitMagicV8;
   const bool v7=v8||u32(data)==kKitMagicV7;
   const bool v6=v7||u32(data)==kKitMagicV6;
   const bool v5=v6||u32(data)==kKitMagicV5;
   if(v5&&size<8+2+4+kCells+6*kRows+4)return false;
   if(v6&&size<8+2+4+2*kRows+kCells+6*kRows+4)return false;
   if(v7&&size<8+2+4+3*kRows+kCells+6*kRows+4)return false;
   if(v8&&size<8+2+4+4*kRows+kCells+6*kRows+4)return false;
   if(v9&&size<8+2+4+4*kRows+1+kCells+6*kRows+4)return false;
   if(v10&&size<8+2+4+4*kRows+2+kCells+6*kRows+4)return false;
   if(v11&&size<8+2+4+5*kRows+2+kCells+6*kRows+4)return false;
   if(v12&&size<8+2+4+5*kRows+2+130+kCells+6*kRows+4)return false;
   if(v13&&size<8+2+4+5*kRows+2+130+kRows+1+kCells+6*kRows+4)return false;
   if(u32(data+size-4)!=hash(data+8,size-12))return false;
   try {
      KitState candidate;
      std::size_t pos=8;
      candidate.selected=data[pos++];
      candidate.visible=data[pos++];
      if(v5){candidate.muted=u16(data+pos);pos+=2;
             candidate.solo=u16(data+pos);pos+=2;}
      if(v6){for(auto& x:candidate.gain)x=data[pos++];
             for(auto& x:candidate.pan)x=data[pos++];}
      if(v7)for(auto& x:candidate.pitch)x=data[pos++];
      if(v8)for(auto& x:candidate.choke)x=data[pos++];
       if(v11)for(auto& x:candidate.output)x=data[pos++];
      if(v9)candidate.swing=data[pos++];
      if(v10)candidate.humanize=data[pos++];
      if(v12){
        candidate.songEnabled=data[pos++];
        const auto count=static_cast<std::size_t>(data[pos++]);
        if(candidate.songEnabled>1||!candidate.chain.setCount(count))return false;
        for(std::size_t i=0;i<SongChain::kCapacity;++i){
          const auto pattern=data[pos++],steps=data[pos++];
          if(i<count&&!candidate.chain.configure(i,pattern,steps))return false;
        }
      }
      if(v13){
        candidate.masterVolume=data[pos++];
        for(auto& amount:candidate.randomVelocity)amount=data[pos++];
      }
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
