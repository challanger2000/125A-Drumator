#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace drumator {
struct UiCompactLayout final {
 static constexpr double width=1280.,header=160.,baseRow=130.,velocityRow=30.,footer=54.;
 static constexpr std::size_t perPage=4;
 static constexpr double headerButtonTop=101.,headerButtonBottom=133.,
      groupTitleTop=49.,groupTitleBottom=65.,globalKnobY=119.,
      songEditorHeight=432.,patternStart=457.,patternPitch=49.,
      patternButtonWidth=47.;
 static constexpr double stepX=622.,stepWidth=19.;
 static constexpr double knobRadius=25.,rowKnobY=88.;
 // Render geometry contract: labels clear the mounted 30px ring by 3px.
 static constexpr double rowKnobMountRadius=30.,rowKnobLabelTop=-46.,
                         rowKnobLabelBottom=-33.;
 static_assert(rowKnobLabelBottom <= -rowKnobMountRadius-3.);
 static_assert(rowKnobY+rowKnobMountRadius < 120.);
 static constexpr double scrollLeft=1263.,scrollRight=1279.,scrollTop=header+7.;
 struct Slot {double left,right;};
 static constexpr Slot barOne{273.,356.},barTwo{359.,442.},edit{208.,272.};
 static constexpr Slot patternTools{850.,966.},songToggle{969.,1038.},
                       songEdit{1041.,1120.};
 static constexpr Slot select{16.,74.},sample{78.,205.},sampleMenu{275.,301.},previous{303.,329.},
   next{332.,358.},preview{362.,398.},fill{403.,452.},mute{457.,491.},
   solo{496.,530.},choke{570.,615.},output{535.,615.},
   velocityToggle{1233.,1259.},rowAdd{16.,110.},rowRemove{116.,210.};
 static constexpr std::array<double,4> rowKnobCenters{{133.,254.,375.,496.}};
 static constexpr std::array<double,3> globalKnobCenters{{79.,177.,1180.}};
 static_assert(barTwo.right<patternStart);
 static_assert(patternStart+7.*patternPitch+patternButtonWidth<patternTools.left);
 static_assert(patternTools.right<songToggle.left&&songToggle.right<songEdit.left);
 static_assert(songEdit.right<globalKnobCenters[2]-50.);
 static constexpr std::array<double,4> userZooms{{1.,1.2,1.3,1.5}};
 static constexpr bool hit(double x,Slot slot) noexcept {return x>=slot.left&&x<slot.right;}
 // At the supported 300px minimum, tall modal menus must extend over the
 // header. Keep every item visible and std::clamp's lower <= upper contract.
 static constexpr double popupTop(double rowY,double clientHeight,double requiredHeight) noexcept {
   const double upper=std::max(0.,clientHeight-requiredHeight);
   return std::clamp(rowY+36.,std::min(header,upper),upper);
 }
 static constexpr double rowTop(std::size_t first,std::size_t index,
                                  const std::array<bool,16>& expanded) noexcept {
   double y=header;
   for(std::size_t i=first;i<first+index&&i<expanded.size();++i)
     y+=baseRow+(expanded[i]?velocityRow:0.);
   return y;
 }
 static constexpr double rowBottom(std::size_t first,std::size_t count,
                                    const std::array<bool,16>& expanded) noexcept {
   return rowTop(first,count,expanded);
 }
 static constexpr double editorHeight(std::size_t first,std::size_t count,
                                      const std::array<bool,16>& expanded) noexcept {
   return rowBottom(first,count,expanded)+footer;
 }
 static constexpr std::size_t rowAt(double y,std::size_t first,std::size_t count,
                                     const std::array<bool,16>& expanded) noexcept {
   for(std::size_t i=0;i<count;++i) {
     const auto t=rowTop(first,i,expanded);
     const auto n=t+baseRow+(expanded[first+i]?velocityRow:0.);
     if(y>=t&&y<n)return i;
   }
   return count;
 }
 static constexpr int stepAt(double x) noexcept {
   return x>=stepX&&x<stepX+32.*stepWidth?
          static_cast<int>((x-stepX)/stepWidth):-1;
 }
 static constexpr bool mainHit(double y,double rowY) noexcept {
   return y>=rowY&&y<rowY+baseRow;
 }
 static constexpr bool velocityHit(double y,double rowY) noexcept {
   return y>=rowY+baseRow&&y<rowY+baseRow+velocityRow;
 }
 static std::uint8_t velocityAt(double y,double rowY,bool erase=false) noexcept {
   if(erase)return 0;
   const auto fraction=std::clamp((rowY+baseRow+velocityRow-3.-y)/(velocityRow-6.),0.,1.);
   return static_cast<std::uint8_t>(1+std::lround(fraction*126.));
 }
 static int knobValue(double startY,double movedY,int initial,int minimum,int maximum) noexcept {
   // 2.5 logical pixels per discrete controller step, extra range clamped.
   return std::clamp(initial+static_cast<int>(std::lround((startY-movedY)/2.5)),minimum,maximum);
 }
 // Use the value's range to choose sensible drag sensitivity, rather
 // than forcing users to drag 500px just to traverse a 0..200% control.
 // Fine mode is latched at mouse-down, so adding/removing Shift mid-gesture
 // cannot unexpectedly jump the value.
 static constexpr double pixelsPerStep(int slot,bool fine=false) noexcept {
   const double unit=slot==2||slot==4?2.5:1.5;
   return fine?unit*4.:unit;
 }
 static int knobValueForSlot(int slot,double startY,double movedY,int initial,
                             bool fine=false) noexcept {
   const auto delta=static_cast<int>(std::lround(
       (startY-movedY)/pixelsPerStep(slot,fine)));
   return std::clamp(initial+delta,minValue(slot),maxValue(slot));
 }
 static constexpr int defaultValue(int slot) noexcept {
   return slot==0||slot==6?100:0;
 }
 static constexpr int minValue(int slot) noexcept {
   return slot==1?-100:slot==2?-24:0;
 }
 static constexpr int maxValue(int slot) noexcept {
   return slot==0||slot==6?200:slot==4?50:slot==1||slot==3||slot==5?100:24;
 }
};
}
