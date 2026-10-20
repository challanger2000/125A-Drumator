#include "core/UiCompactLayout.h"
#include "core/UiSongLayout.h"
#include <iostream>
using drumator::UiCompactLayout;
using drumator::UiSongLayout;
#define EXPECT(v) do{if(!(v)){std::cerr<<__LINE__<<" " #v "\n";return __LINE__;}}while(false)
int main(){
 std::array<bool,16> expanded{};
 EXPECT(UiCompactLayout::editorHeight(0,4,expanded)==538.);
 expanded[0]=true;expanded[2]=true;
 EXPECT(UiCompactLayout::rowTop(0,1,expanded)==232.);
 EXPECT(UiCompactLayout::rowTop(0,2,expanded)==326.);
 EXPECT(UiCompactLayout::rowTop(0,3,expanded)==450.);
 EXPECT(UiCompactLayout::editorHeight(0,4,expanded)==598.);
 EXPECT(UiCompactLayout::rowAt(233.,0,4,expanded)==1);
 EXPECT(UiCompactLayout::rowAt(325.,0,4,expanded)==1);
 EXPECT(UiCompactLayout::rowAt(449.,0,4,expanded)==2);
 EXPECT(UiCompactLayout::rowAt(650.,0,4,expanded)==4);
 EXPECT(UiCompactLayout::velocityHit(205.,108.));
 EXPECT(!UiCompactLayout::velocityHit(201.,108.));
 EXPECT(UiCompactLayout::stepAt(622.)==0);
 EXPECT(UiCompactLayout::stepAt(1229.9)==31);
 EXPECT(UiCompactLayout::stepAt(1230.)==-1);
 EXPECT(UiCompactLayout::velocityAt(202.,108.)==127);
 EXPECT(UiCompactLayout::velocityAt(232.,108.)==1);
 expanded.fill(true);
 EXPECT(UiCompactLayout::editorHeight(12,4,expanded)==658.);
 EXPECT(UiCompactLayout::knobValue(100.,0.,100,0,200)==140);
 EXPECT(UiCompactLayout::knobValue(100.,1000.,100,0,200)==0);
 EXPECT(UiCompactLayout::knobValue(100.,-1000.,100,0,200)==200);
 EXPECT(UiCompactLayout::maxValue(0)==200);
 EXPECT(UiCompactLayout::maxValue(2)==24);
 EXPECT(UiCompactLayout::defaultValue(0)==100);
 EXPECT(UiCompactLayout::hit(275.,UiCompactLayout::sampleMenu));
 EXPECT(!UiCompactLayout::hit(274.9,UiCompactLayout::sampleMenu));
 EXPECT(!UiCompactLayout::hit(301.,UiCompactLayout::sampleMenu));
 EXPECT(UiCompactLayout::sample.right<UiCompactLayout::sampleMenu.left);
 EXPECT(UiCompactLayout::sampleMenu.right<UiCompactLayout::previous.left);
 EXPECT(UiCompactLayout::output.right-UiCompactLayout::output.left>=80.);
 EXPECT(UiCompactLayout::output.left==535.);
 EXPECT(UiCompactLayout::choke.left==570.);
 EXPECT(UiCompactLayout::maxValue(4)==50);
 EXPECT(UiCompactLayout::defaultValue(4)==0);
 EXPECT(UiCompactLayout::minValue(2)==-24);
 // Song geometry is shared by VSTGUI painting and pointer input.
 EXPECT(UiSongLayout::perPage==16);
 EXPECT(UiSongLayout::pageOf(0)==0);
 EXPECT(UiSongLayout::pageOf(16)==1);
 EXPECT(UiSongLayout::pageOf(63)==3);
 EXPECT(UiSongLayout::slotAt(16.,145.,0)==0);
 EXPECT(UiSongLayout::slotAt(17.,198.,0)==8);
 EXPECT(UiSongLayout::slotAt(1096.,202.,2)==47);
 EXPECT(UiSongLayout::slotAt(16.,145.,1)==16);
 EXPECT(UiSongLayout::slotAt(167.,160.,0)==drumator::SongChain::kCapacity);
 EXPECT(UiSongLayout::slotAt(15.,145.,0)==drumator::SongChain::kCapacity);
 EXPECT(UiSongLayout::slotAt(18.,191.,0)==drumator::SongChain::kCapacity);
 EXPECT(UiSongLayout::slotAt(18.,249.,0)==drumator::SongChain::kCapacity);
 EXPECT(UiSongLayout::slotAt(18.,160.,4)==drumator::SongChain::kCapacity);
 EXPECT(UiSongLayout::previousHit(1000.,119.));
 EXPECT(UiSongLayout::nextHit(1130.,119.));
 EXPECT(!UiSongLayout::nextHit(1105.,119.));
 EXPECT(!UiSongLayout::canNext(0,15));
 EXPECT(UiSongLayout::canNext(0,16));
 EXPECT(!UiSongLayout::canNext(3,64));

 // Drag sensitivity must be scaled to each parameter's numerical range.
 EXPECT(UiCompactLayout::knobValueForSlot(0,100.,75.,100)==117);
 EXPECT(UiCompactLayout::knobValueForSlot(0,100.,85.,100,true)==103);
 EXPECT(UiCompactLayout::knobValueForSlot(0,100.,85.,100,false)==110);
 EXPECT(UiCompactLayout::knobValueForSlot(2,100.,40.,0)==24);
 EXPECT(UiCompactLayout::knobValueForSlot(4,100.,-25.,0)==50);
 EXPECT(UiCompactLayout::knobValueForSlot(6,100.,-1000.,100)==200);
 EXPECT(UiCompactLayout::knobValueForSlot(1,100.,1000.,0)==-100);
 EXPECT(UiCompactLayout::rowAdd.right<=UiCompactLayout::rowRemove.left);
 EXPECT(UiCompactLayout::hit(76.,UiCompactLayout::rowAdd));
 EXPECT(UiCompactLayout::hit(150.,UiCompactLayout::rowRemove));
 EXPECT(!UiCompactLayout::hit(113.,UiCompactLayout::rowAdd));


 return 0;
}
