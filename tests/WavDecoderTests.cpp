#include "core/WavPcm16Decoder.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>
using namespace drumator;
static void require(bool x,const char* msg){if(!x)throw std::runtime_error(msg);}
static void u16(std::vector<std::uint8_t>& a,std::uint16_t v){
 a.push_back(v&255u);a.push_back((v>>8)&255u);
}
static void u32(std::vector<std::uint8_t>& a,std::uint32_t v){
 for(int i=0;i<4;++i)a.push_back((v>>(i*8))&255u);
}
static void id(std::vector<std::uint8_t>& a,const char* p){
 for(int i=0;i<4;++i)a.push_back(static_cast<std::uint8_t>(p[i]));
}
static void fix32(std::vector<std::uint8_t>& a,std::size_t p,std::uint32_t v){
 for(int i=0;i<4;++i)a[p+i]=(v>>(8*i))&255u;
}
static std::vector<std::uint8_t> wav(bool stereo) {
 std::vector<std::uint8_t> v;
 id(v,"RIFF");u32(v,0);id(v,"WAVE");
 id(v,"JUNK");u32(v,3);v.insert(v.end(),{1,2,3,0});
 id(v,"data");u32(v,stereo?12:6);
 // Place data before fmt to verify chunk-order independent parsing.
 for(std::int16_t s:std::array<std::int16_t,6>{-32768,-16384,0,16384,32767,-1}) {
   if(stereo)u16(v,static_cast<std::uint16_t>(s));
   else if(s==-32768||s==0||s==32767)u16(v,static_cast<std::uint16_t>(s));
 }
 id(v,"fmt ");u32(v,16);u16(v,1);u16(v,stereo?2:1);
 u32(v,48000);u32(v,48000*(stereo?4:2));u16(v,stereo?4:2);u16(v,16);
 fix32(v,4,static_cast<std::uint32_t>(v.size()-8));
 return v;
}
int main(){
 try {
  DecodedWav result;
  const auto mono=wav(false);
  require(WavPcm16Decoder::decode(mono.data(),mono.size(),result)==WavError::Ok,
          "mono decoder rejected legal RIFF");
  require(result.channels==1&&result.sampleRate==48000&&
          result.left.size()==3&&result.right.empty(),"mono header");
  require(result.left[0]==-1.f&&result.left[1]==0.f,"signed 16-bit endpoints");
  require(std::abs(result.left[2]-32767.f/32768.f)<1e-8,"PCM positive fullscale");
  const auto stereo=wav(true);
  require(WavPcm16Decoder::decode(stereo.data(),stereo.size(),result)==WavError::Ok,
          "stereo decoder rejected legal RIFF");
  require(result.channels==2&&result.left.size()==3&&result.right.size()==3,
          "stereo frame count");
  require(result.left[0]==-1.f&&result.right[0]==-0.5f,"stereo first");
  require(result.left[1]==0.f&&result.right[1]==0.5f,"stereo second");
  require(std::abs(result.left[2]-32767.f/32768.f)<1e-8&&
          result.right[2]==-1.f/32768.f,"stereo third");
  const auto baseline=result.left;
  auto bad=stereo;bad[0]='R';bad[1]='F';bad[2]='6';bad[3]='4';
  require(WavPcm16Decoder::decode(bad.data(),bad.size(),result)==
          WavError::UnsupportedFormat,"RF64 falsely accepted");
  bad=stereo;fix32(bad,4,static_cast<std::uint32_t>(bad.size()+200));
  require(WavPcm16Decoder::decode(bad.data(),bad.size(),result)==
          WavError::Truncated,"truncated RIFF accepted");
  bad=stereo;bad.pop_back();
  require(WavPcm16Decoder::decode(bad.data(),bad.size(),result)==
          WavError::Truncated,"missing final byte");
  bad=stereo;
  // fmt follows data: calculate its start from last 24 bytes.
  const auto fmt=bad.size()-24;
  bad[fmt+8]=3;
  require(WavPcm16Decoder::decode(bad.data(),bad.size(),result)==
          WavError::UnsupportedFormat,"float format accepted as PCM16");
  bad=stereo;bad[fmt+20]=5;
  require(WavPcm16Decoder::decode(bad.data(),bad.size(),result)==
          WavError::InvalidFormat,"invalid block align accepted");
  bad=stereo;
  // Corrupt the declared data size to break integral frames while
  // retaining legal overall RIFF shape.
  const std::size_t dataChunk=12+8+4;
  bad[dataChunk+4]=11;
  require(WavPcm16Decoder::decode(bad.data(),bad.size(),result)!=WavError::Ok,
          "invalid stereo frames accepted");
  require(result.left==baseline&&result.channels==2,
          "failed decode changed prior valid sound");
  require(WavPcm16Decoder::decode(nullptr,stereo.size(),result)!=WavError::Ok,
          "null bytes accepted");
  std::cout<<"WAV PCM16 decoder regressions PASS\n";
  return 0;
 } catch(const std::exception& e){
  std::cerr<<"WAV PCM16 decoder FAIL: "<<e.what()<<'\n';
  return 1;
 }
}
