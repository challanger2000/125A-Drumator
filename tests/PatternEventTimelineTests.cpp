#include "core/PatternEventTimeline.h"
#include <iostream>
#define CHECK(expr) do {if(!(expr)){std::cerr<<"Timeline:"<<__LINE__<<" " #expr "\n";return __LINE__;}}while(false)
int main(){
 drumator::PatternEventTimeline t;
 CHECK(t.configure(512)&&t.reset(512));
 for(int i=0;i<257;++i)t.insert(i,1,false);
 CHECK(t.get(0).pattern==1&&t.get(255).pattern==1&&t.get(256).pattern==1);
 CHECK(t.get(257).source==0);
 t.insert(64,2,true);t.insert(64,3,false);
 CHECK(t.get(64).pattern==2&&t.get(64).source==drumator::PatternEventTimeline::kHost);
 t.insert(64,4,true);CHECK(t.get(64).pattern==4);
 CHECK(t.reset(256));
 for(int i=0;i<2048;++i)t.insert(32,static_cast<unsigned>(i%8),false);
 CHECK(t.get(32).pattern==2047%8);
 CHECK(!t.reset(513)&&t.get(32).source==drumator::PatternEventTimeline::kMidi);
 CHECK(t.reset(256)&&t.get(32).source==0);
 t.insert(-1,1,true);t.insert(256,1,true);t.insert(0,8,true);
 CHECK(t.get(0).source==0);
 return 0;
}
