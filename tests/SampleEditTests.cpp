#include "core/SampleEdit.h"
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
 std::cout<<"SampleEdit core geometry/envelope PASS\\n";
}
