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
