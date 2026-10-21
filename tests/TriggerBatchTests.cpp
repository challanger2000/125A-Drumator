#include "core/TriggerBatch.h"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace drumator;
static void need(bool condition,const char* msg){if(!condition)throw std::runtime_error(msg);}
int main(){
 try {
  TriggerBatch batch;
  need(!batch.appendNoteOn(35,1.0f,0,64),"low pitch accepted");
  need(!batch.appendNoteOn(52,1.0f,0,64),"high pitch accepted");
  need(!batch.appendNoteOn(36,0.0f,0,64),"zero velocity accepted");
  need(!batch.appendNoteOn(36,-.2f,0,64),"negative velocity accepted");
  need(!batch.appendNoteOn(36,2.0f,0,64),"overvelocity accepted");
  need(!batch.appendNoteOn(36,std::numeric_limits<float>::quiet_NaN(),0,64),"NaN accepted");
  need(!batch.appendNoteOn(36,1.0f,-1,64),"negative offset accepted");
  need(!batch.appendNoteOn(36,1.0f,64,64),"outside half open block accepted");
  need(batch.appendNoteOn(51,1.0f,50,64),"last row rejected");
  need(batch.appendNoteOn(36,.5f,7,64),"first row rejected");
  need(batch.appendStep({7,7,3,127,1}),"step append");
  batch.sort();
  need(batch.size()==3,"event count");
  need(batch.data()[0].sampleOffset==7&&batch.data()[0].row==0&&
       batch.data()[0].velocity==64,"velocity rounding or first row wrong");
  need(batch.data()[1].sampleOffset==7&&batch.data()[1].row==7,
       "sorted same sample offset wrong");
  need(batch.data()[2].sampleOffset==50&&batch.data()[2].row==15&&
       batch.data()[2].velocity==127,"last row wrong");
  batch.applyMuteSolo(1u<<7,0);
  need(batch.size()==2&&batch.data()[0].row==0&&batch.data()[1].row==15,
       "mute did not filter expected row");
  batch.applyMuteSolo(0,1u<<15);
  need(batch.size()==1&&batch.data()[0].row==15,"solo isolation failed");
  // RND velocity: 0% is bit-identical and enabled amounts never create
  // triggers or modify stored patterns. A callback split must not alter hits.
  batch.clear();
  const Trigger reference{6100,3,0,100,1};
  need(batch.appendStep(reference),"random test append");
  std::array<std::uint8_t,kRows> depth{};
  batch.varyVelocity(depth,8.0,120.0,48000.0);
  need(batch.data()[0].velocity==100,"random zero must preserve velocity");
  depth[3]=100;
  batch.varyVelocity(depth,8.0,120.0,48000.0);
  const auto first=batch.data()[0].velocity;
  need(first>=1&&first<=127,"random must stay in MIDI velocity range");
  batch.clear();
  need(batch.appendStep({100,3,0,100,1}),"split callback append");
  // Same absolute instant: 6100 in the full block equals 100 after a
  // 6000-frame split. This crosses a musical-step boundary; the old fixture
  // advanced both start and offset by 100 while remaining in the same step.
  const double ppq=8.0+6000.0*120.0/(60.0*48000.0);
  batch.varyVelocity(depth,ppq,120.0,48000.0);
  need(batch.data()[0].velocity==first,"random changed across host blocks");
  batch.clear();
  need(batch.appendStep({100,3,0,100,1}),"wrong-time negative append");
  batch.varyVelocity(depth,8.0,120.0,48000.0);
  need(batch.data()[0].velocity!=first,"random partition oracle cannot detect a different absolute step");
  batch.clear();
  need(batch.appendStep(reference),"random fixed-seed append");
  batch.varyVelocity(depth,8.0,120.0,48000.0);
  need(batch.data()[0].velocity==first,"random value must be reproducible");
  batch.clear();
  need(batch.size()==0,"clear");
  for(std::size_t i=0;i<TriggerBatch::kCapacity;++i)
    need(batch.appendNoteOn(36,1.0f,0,64),"premature capacity");
  need(!batch.appendNoteOn(36,1.0f,0,64),"capacity overflow");
  batch.sort();
  need(batch.size()==TriggerBatch::kCapacity,"lost buffered entries");
  std::cout<<"Drumator MIDI trigger batch PASS\n";
  return 0;
 }catch(const std::exception& ex) {
  std::cerr<<"Drumator MIDI trigger batch FAIL: "<<ex.what()<<'\n';
  return 1;
 }
}
