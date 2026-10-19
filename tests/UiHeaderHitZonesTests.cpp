#include "core/UiHeaderHitZones.h"
#include <iostream>
using drumator::UiHeaderHitZones;
#define CHECK(expr) do { if (!(expr)) {std::cerr<<__LINE__<<": " #expr "\n";return __LINE__;} }while(false)
int main() {
 CHECK(UiHeaderHitZones::masterMinus(1140,74));
 CHECK(UiHeaderHitZones::masterMinus(1179.999,97.999));
 CHECK(!UiHeaderHitZones::masterMinus(1180,80));
 CHECK(!UiHeaderHitZones::masterPlus(1183.999,80));
 CHECK(UiHeaderHitZones::masterPlus(1184,80));
 CHECK(UiHeaderHitZones::masterPlus(1227.999,97.999));
 CHECK(!UiHeaderHitZones::masterPlus(1228,80));
 CHECK(!UiHeaderHitZones::masterPlus(1190,98));
 CHECK(!UiHeaderHitZones::masterMinus(1140,73.999));
 CHECK(UiHeaderHitZones::songToggle(540,74));
 CHECK(UiHeaderHitZones::songToggle(637.999,97.999));
 CHECK(!UiHeaderHitZones::songToggle(638,80));
 CHECK(!UiHeaderHitZones::songEditor(651.999,80));
 CHECK(UiHeaderHitZones::songEditor(652,74));
 CHECK(UiHeaderHitZones::songEditor(849.999,97.999));
 CHECK(!UiHeaderHitZones::songEditor(850,80));
 CHECK(!UiHeaderHitZones::songToggle(610,98));
 for(int x=638;x<652;++x)
   CHECK(!UiHeaderHitZones::songToggle(x,80)&&!UiHeaderHitZones::songEditor(x,80));
 for(int x=1180;x<1184;++x)
   CHECK(!UiHeaderHitZones::masterMinus(x,80)&&!UiHeaderHitZones::masterPlus(x,80));
 return 0;
}
