#include "core/UiOverlayHitZones.h"
#include <iostream>
#define CHECK(x) do {if(!(x)){std::cerr<<"Overlay:"<<__LINE__<<" " #x "\n";return __LINE__;}}while(false)
int main(){
 using drumator::UiOverlayHitZones;
 CHECK(UiOverlayHitZones::canDrop(700,150,4,false,false,0));
 CHECK(!UiOverlayHitZones::canDrop(700,150,4,true,false,0));
 CHECK(!UiOverlayHitZones::canDrop(700,150,4,false,true,108));
 CHECK(UiOverlayHitZones::canDrop(900,150,4,false,true,108));
 CHECK(!UiOverlayHitZones::canDrop(700,224,4,false,false,0));
 return 0;
}
