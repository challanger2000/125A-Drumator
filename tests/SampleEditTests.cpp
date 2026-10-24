#include "core/SampleEdit.h"
#include "core/WaveformExtremaIndex.h"
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <cmath>
#include <iostream>
using drumator::SampleEdit;
void require(bool ok){if(!ok)throw std::runtime_error("SampleEdit regression failed");}
int main(){
 auto e=SampleEdit::reset(1000);
 require(e.valid(1000)&&e.length()==1000);
 require(e.amplitude(0)==1.f&&e.amplitude(999)==1.f);
 e.start=100;e.end=900;e.fadeIn=100;e.fadeOut=100;e.gain=.5f;
 require(e.valid(1000)&&e.length()==800);
 require(e.amplitude(99)==0.f&&e.amplitude(900)==0.f);
 require(e.amplitude(100)==0.f&&e.amplitude(200)==.5f);
 require(e.amplitude(899)==0.f);
 require(std::abs(e.amplitude(850)-.245f)<.001f);
 e.fadeIn=500;e.fadeOut=400;require(!e.valid(1000));
 e=SampleEdit::reset(1000);e.gain=std::numeric_limits<float>::infinity();
 require(!e.valid(1000));
 const float data[]{-.5f,.7f,.2f,-.9f};
 auto range=drumator::waveformRange(data,4,1,4);
 require(range.minimum==-.9f&&range.maximum==.7f);
 require(drumator::waveformFrameAt(-10,200,0,100)==0);
 require(drumator::waveformFrameAt(100,200,0,100)==50);
 require(drumator::waveformFrameAt(220,200,0,100)==100);
 // Preview/OK/CANCEL share immutable, validated edit envelopes.
 auto pending=SampleEdit::reset(8192);
 pending.start=256;pending.end=4096;pending.fadeIn=256;
 pending.fadeOut=128;pending.gain=.75f;
 require(pending.valid(8192));
 const auto saved=SampleEdit::reset(8192);
 // CANCEL preserves the saved state; only OK would select pending.
 require(saved.start==0&&saved.end==8192&&saved.gain==1.f);
 require(pending.amplitude(256)==0.f);
 require(std::abs(pending.amplitude(512)-.75f)<1e-6f);
 require(pending.amplitude(4095)==0.f);
 pending.end=pending.start+1;
 pending.fadeIn=0;pending.fadeOut=0;
 require(pending.valid(8192)&&pending.length()==1);
 pending.fadeIn=1;pending.fadeOut=1;
 require(!pending.valid(8192));
 pending=SampleEdit::reset(8192);
 pending.gain=0.f;
 require(pending.valid(8192)&&pending.amplitude(512)==0.f);
 pending.gain=-1.f;require(!pending.valid(8192));
 pending.gain=4.01f;require(!pending.valid(8192));
 require(drumator::waveformFrameAt(355.,710.,200,1200)==700);
 // Exact independent reference using the original one-pass extrema.
 const std::array<std::size_t,13> sizes{{1u,2u,3u,255u,256u,257u,
                                        511u,512u,513u,1024u,8193u,131089u,65537u}};
 for(const auto length:sizes) {
   std::vector<float> pcm(length);
   std::uint32_t seed=0xA1B2C3D4u;
   for(std::size_t i=0;i<length;++i) {
     seed=seed*1664525u+1013904223u;
     pcm[i]=static_cast<float>(static_cast<int>(seed%4001u)-2000)/2000.f;
   }
   for(std::size_t i=255u;i<length;i+=256u)
     pcm[i]=(i%2u==0u)?1.f:-1.f;
   if(length>1u){pcm[0]=-0.f;pcm[1]=+0.f;}
   drumator::WaveformExtremaIndex index;
   require(index.build(pcm.data(),pcm.size())&&
           index.blockCount()==length/256u+(length%256u!=0u?1u:0u));
   const auto compare=[&](std::size_t first,std::size_t last){
     const auto oracle=drumator::waveformRange(pcm.data(),pcm.size(),first,last);
     const auto actual=index.range(pcm.data(),pcm.size(),first,last);
     require(std::memcmp(&oracle.minimum,&actual.minimum,sizeof(float))==0&&
             std::memcmp(&oracle.maximum,&actual.maximum,sizeof(float))==0);
   };
   compare(0,length);compare(length-1,length);
   for(std::size_t i=0;i<length;i+=std::max(std::size_t{1},length/300u)){
     const auto span=std::min(length-i,std::size_t{8193});
     const auto last=i+1u+(i*104729u)%span;
     compare(i,last);
   }
   for(std::size_t i=0;i<length;i+=std::max(std::size_t{1},length/50u))
     if(length-i>1u)compare(i,i+(length-i)/2u);
   require(index.range(pcm.data(),pcm.size(),length,length).minimum==0.f);
   require(index.range(pcm.data(),pcm.size(),0,length+1u).maximum==0.f);
   require(index.range(nullptr,pcm.size(),0,length).maximum==0.f);
   index.clear();
   require(index.blockCount()==0u);
   compare(0,length);
 }
 std::cout<<"SampleEdit indexed extrema match legacy PCM oracle PASS\\n";
 std::cout<<"SampleEdit core geometry/envelope PASS\\n";
}
