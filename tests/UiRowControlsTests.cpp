#include "core/UiRowControls.h"
#include <iostream>
#include <array>
using drumator::UiRowControls;
#define CHECK(expr) do {if(!(expr)){std::cerr<<__LINE__<<" " #expr "\n";return __LINE__;}}while(false)
int main(){
 constexpr std::array<UiRowControls::Slot,11> slots{{
  UiRowControls::fill,UiRowControls::preview,UiRowControls::sample,
  UiRowControls::previous,UiRowControls::next,UiRowControls::output,
  UiRowControls::volume,UiRowControls::pan,UiRowControls::pitch,
  UiRowControls::mute,UiRowControls::solo}};
 for(std::size_t i=0;i<slots.size();++i){
  CHECK(slots[i].right>slots[i].left+20.);
  CHECK(UiRowControls::hit(slots[i],slots[i].left));
  CHECK(UiRowControls::hit(slots[i],slots[i].right-0.001));
  CHECK(!UiRowControls::hit(slots[i],slots[i].right));
  CHECK(slots[i].right<=622.);
  if(i) {
   CHECK(slots[i-1].right<slots[i].left);
   CHECK(!UiRowControls::hit(slots[i],slots[i-1].right));
  }
 }
 CHECK((UiRowControls::sample.right-UiRowControls::sample.left)>=160.);
 CHECK((UiRowControls::previous.right-UiRowControls::previous.left)>=24.);
 CHECK((UiRowControls::next.right-UiRowControls::next.left)>=24.);
 CHECK(UiRowControls::mid(UiRowControls::volume)==414.5);
 return 0;
}
