// 125A Drumator: bit-exact waveform min/max acceleration, UI/control thread.
#pragma once
#include "core/SampleEdit.h"
#include <algorithm>
#include <cstddef>
#include <new>
#include <utility>
#include <vector>
namespace drumator {
class WaveformExtremaIndex {
public:
 static constexpr std::size_t kBlockFrames=256;
 bool build(const float* samples,std::size_t frames) {
   if(!samples||frames==0)return false;
   const auto blocks=frames/kBlockFrames+(frames%kBlockFrames!=0u?1u:0u);
   std::vector<WaveformRange> stage;
   try {stage.resize(blocks);}catch(const std::bad_alloc&){return false;}
   for(std::size_t i=0;i<blocks;++i) {
     const auto begin=i*kBlockFrames;
     stage[i]=waveformRange(samples,frames,begin,
                            std::min(frames,begin+kBlockFrames));
   }
   blocks_.swap(stage);frames_=frames;return true;
 }
 void clear() noexcept {blocks_.clear();frames_=0;}
 std::size_t blockCount() const noexcept {return blocks_.size();}
 WaveformRange range(const float* samples,std::size_t frames,
                     std::size_t begin,std::size_t end) const noexcept {
   if(!samples||!frames||begin>=end||end>frames)return {};
   if(frames!=frames_||blocks_.empty())
     return waveformRange(samples,frames,begin,end);
   const auto firstEnd=std::min(end,(begin/kBlockFrames+1u)*kBlockFrames);
   WaveformRange out=waveformRange(samples,frames,begin,firstEnd);
   auto at=firstEnd;
   while(end-at>=kBlockFrames) {
     const auto& block=blocks_[at/kBlockFrames];
     out.minimum=std::min(out.minimum,block.minimum);
     out.maximum=std::max(out.maximum,block.maximum);
     at+=kBlockFrames;
   }
   if(at<end) {
     const auto tail=waveformRange(samples,frames,at,end);
     out.minimum=std::min(out.minimum,tail.minimum);
     out.maximum=std::max(out.maximum,tail.maximum);
   }
   return out;
 }
private:
 std::size_t frames_{0};
 std::vector<WaveformRange> blocks_;
};
}
