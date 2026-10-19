#include "core/SongChain.h"
#include <iostream>
#define CHECK(cond) do { if (!(cond)) { \
 std::cerr << "SongChain check failed at " << __LINE__ << ": " #cond << '\n'; \
 return __LINE__; } } while(false)
int main(){
 using drumator::SongChain;
 SongChain c;
#ifdef DRUMATOR_TEST_EXPECTED_FAILURE
 CHECK(false); // CTest WILL_FAIL validates that Release checks are active.
#endif
 CHECK(c.totalSteps()==256);
 for(int i=0;i<8;++i){CHECK(c.at(i*32).pattern==i);CHECK(c.at(i*32+31).step==31);}
 CHECK(!c.setCount(0)&&!c.setCount(65));
 CHECK(c.setCount(3));
 CHECK(c.configure(0,0,16));
 CHECK(c.configure(1,1,8));
 CHECK(c.configure(2,2,32));
 CHECK(!c.configure(3,0,16)&&!c.configure(0,8,16)&&!c.configure(0,0,7));
 CHECK(c.totalSteps()==56);
 CHECK(c.at(15).pattern==0);
 CHECK(c.at(16).pattern==1&&c.at(16).step==0);
 CHECK(c.at(23).step==7);
 CHECK(c.at(24).pattern==2&&c.at(24).step==0);
 CHECK(c.at(55).step==31);
 CHECK(c.at(56).pattern==0);
 CHECK(c.at(-1).pattern==2&&c.at(-1).step==31);
 CHECK(c.at(117,101).pattern==1);
 CHECK(c.setCount(4));
 CHECK(c.entry(3).pattern==0&&c.entry(3).steps==32);
}
