#include "core/UiCompactLayout.h"
#include "core/UiWaveEditorLayout.h"
#include "core/UiSongLayout.h"
#include <iostream>
using drumator::UiCompactLayout;
using drumator::UiSongLayout;
#define EXPECT(v) do{if(!(v)){std::cerr<<__LINE__<<" " #v "\n";return __LINE__;}}while(false)
int main(){
 std::array<bool,16> expanded{};
 EXPECT(UiCompactLayout::editorHeight(0,4,expanded)==682.);
 expanded[0]=true;expanded[2]=true;
 EXPECT(UiCompactLayout::rowTop(0,1,expanded)==268.);
 EXPECT(UiCompactLayout::rowTop(0,2,expanded)==398.);
 EXPECT(UiCompactLayout::rowTop(0,3,expanded)==558.);
 EXPECT(UiCompactLayout::editorHeight(0,4,expanded)==742.);
 EXPECT(UiCompactLayout::rowAt(269.,0,4,expanded)==1);
 EXPECT(UiCompactLayout::rowAt(397.,0,4,expanded)==1);
 EXPECT(UiCompactLayout::rowAt(557.,0,4,expanded)==2);
 EXPECT(UiCompactLayout::rowAt(750.,0,4,expanded)==4);
 EXPECT(UiCompactLayout::velocityHit(241.,108.));
 EXPECT(!UiCompactLayout::velocityHit(237.,108.));
 EXPECT(UiCompactLayout::stepAt(622.)==0);
 EXPECT(UiCompactLayout::stepAt(1229.9)==31);
 EXPECT(UiCompactLayout::stepAt(1230.)==-1);
 EXPECT(UiCompactLayout::velocityAt(241.,108.)==127);
 EXPECT(UiCompactLayout::velocityAt(271.,108.)==1);
 expanded.fill(true);
 EXPECT(UiCompactLayout::editorHeight(12,4,expanded)==802.);
 EXPECT(UiCompactLayout::knobValue(100.,0.,100,0,200)==140);
 EXPECT(UiCompactLayout::knobValue(100.,1000.,100,0,200)==0);
 EXPECT(UiCompactLayout::knobValue(100.,-1000.,100,0,200)==200);
 EXPECT(UiCompactLayout::maxValue(0)==200);
 EXPECT(UiCompactLayout::maxValue(2)==24);
 EXPECT(UiCompactLayout::defaultValue(0)==100);
 // Regression: added physical knob bezels must never obscure labels, overlap
 // the top-row buttons or cut into the bottom edge of their recessed bay.
 static_assert(UiCompactLayout::rowKnobY+UiCompactLayout::rowKnobLabelTop>=42.);
 static_assert(UiCompactLayout::rowKnobLabelBottom<=-UiCompactLayout::rowKnobMountRadius-3.);
 static_assert(UiCompactLayout::rowKnobY+UiCompactLayout::rowKnobMountRadius<=118.);
 EXPECT(UiCompactLayout::hit(275.,UiCompactLayout::sampleMenu));
 EXPECT(!UiCompactLayout::hit(274.9,UiCompactLayout::sampleMenu));
 EXPECT(!UiCompactLayout::hit(301.,UiCompactLayout::sampleMenu));
 // Reusable component family geometry: paired choices and EDIT must
 // remain fixed-width, non-overlapping, and use the same hit logic as draw.
 EXPECT(UiCompactLayout::barOne.right-UiCompactLayout::barOne.left==84.);
 EXPECT(UiCompactLayout::barTwo.right-UiCompactLayout::barTwo.left==84.);
 EXPECT(UiCompactLayout::barOne.right<UiCompactLayout::barTwo.left);
 EXPECT(UiCompactLayout::barTwo.right<465.); // centered title begins x=465
 EXPECT(UiCompactLayout::edit.right-UiCompactLayout::edit.left==64.);
 EXPECT(UiCompactLayout::sample.right<UiCompactLayout::edit.left);
 EXPECT(UiCompactLayout::edit.right<UiCompactLayout::sampleMenu.left);
 // The paint and hit maps for every WAV toolbar action share one schema.
 using Wave=drumator::UiWaveEditorLayout;
 EXPECT(Wave::toolbar.size()==8);
 EXPECT(Wave::zoomIn.width()==Wave::zoomOut.width());
 EXPECT(Wave::fit.width()==Wave::reset.width());
 EXPECT(Wave::preview.width()==Wave::stop.width());
 EXPECT(Wave::cancel.width()==Wave::apply.width());
 EXPECT(Wave::buttonAt(310.,470.)==0);
 EXPECT(Wave::buttonAt(390.,470.)==1);
 EXPECT(Wave::buttonAt(470.,470.)==2);
 EXPECT(Wave::buttonAt(560.,470.)==3);
 EXPECT(Wave::buttonAt(660.,470.)==4);
 EXPECT(Wave::buttonAt(770.,470.)==5);
 EXPECT(Wave::buttonAt(875.,470.)==6);
 EXPECT(Wave::buttonAt(990.,470.)==7);
 EXPECT(Wave::buttonAt(521.,470.)==-1);
 EXPECT(Wave::gain.hit(650.,420.));
 EXPECT(Wave::waveform.hit(400.,250.));
 EXPECT(Wave::waveform.hit(500.,155.));
 EXPECT(Wave::waveform.hit(500.,165.));
 EXPECT(Wave::waveform.hit(500.,382.));
 EXPECT(!Wave::waveform.hit(500.,154.9));
 EXPECT(!Wave::waveform.hit(500.,382.1));
 EXPECT(Wave::scrollbar.hit(400.,395.));
 EXPECT(UiCompactLayout::hit(240.,UiCompactLayout::edit));
 EXPECT(!UiCompactLayout::hit(240.,UiCompactLayout::sample));
 EXPECT(UiCompactLayout::sample.right<UiCompactLayout::sampleMenu.left);
 EXPECT(UiCompactLayout::sampleMenu.right<UiCompactLayout::previous.left);
 EXPECT(UiCompactLayout::output.right-UiCompactLayout::output.left>=80.);
 EXPECT(UiCompactLayout::output.left==535.);
 EXPECT(UiCompactLayout::choke.left==570.);
 EXPECT(UiCompactLayout::maxValue(4)==50);
 EXPECT(UiCompactLayout::defaultValue(4)==0);
 EXPECT(UiCompactLayout::minValue(2)==-24);
 // Song geometry is shared by VSTGUI painting and pointer input.
 static_assert(UiSongLayout::perPage==16);
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
 // A host may keep the advertised minimum 300px editor instead of accepting
 // automatic growth. All modal items must still fit; no reversed clamp bounds.
 for(double client:{300.,370.,538.,658.,850.})
   for(double rowY:{108.,202.,296.,390.})
     for(double required:{100.,145.,210.,230.}) {
       const auto top=UiCompactLayout::popupTop(rowY,client,required);
       EXPECT(top>=0.&&top+required<=client);
     }
 EXPECT(UiCompactLayout::popupTop(108.,300.,230.)==70.);
 EXPECT(UiCompactLayout::popupTop(108.,300.,210.)==90.);
 EXPECT(UiCompactLayout::popupTop(108.,538.,230.)==144.);


 return 0;
}
