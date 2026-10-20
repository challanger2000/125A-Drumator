#include "core/UiCompactLayout.h"
#include <iostream>
using drumator::UiCompactLayout;
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

 return 0;
}
