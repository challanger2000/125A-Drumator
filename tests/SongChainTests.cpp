#include "core/SongChain.h"
#include <cassert>
int main(){
 using drumator::SongChain;
 SongChain c;
 assert(c.totalSteps()==256);
 for(int i=0;i<8;++i){assert(c.at(i*32).pattern==i);assert(c.at(i*32+31).step==31);}
 assert(!c.setCount(0)&&!c.setCount(65));
 assert(c.setCount(3));
 assert(c.configure(0,0,16));
 assert(c.configure(1,1,8));
 assert(c.configure(2,2,32));
 assert(!c.configure(3,0,16)&&!c.configure(0,8,16)&&!c.configure(0,0,7));
 assert(c.totalSteps()==56);
 assert(c.at(15).pattern==0);
 assert(c.at(16).pattern==1&&c.at(16).step==0);
 assert(c.at(23).step==7);
 assert(c.at(24).pattern==2&&c.at(24).step==0);
 assert(c.at(55).step==31);
 assert(c.at(56).pattern==0);
 assert(c.at(-1).pattern==2&&c.at(-1).step==31);
 assert(c.at(117,101).pattern==1);
 assert(c.setCount(4));
 assert(c.entry(3).pattern==0&&c.entry(3).steps==32);
}
