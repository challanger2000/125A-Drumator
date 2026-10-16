#include "DrumatorView.h"
#include "DrumatorController.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/ccolor.h"
#include <algorithm>
#include <cstdio>
#include <string>

namespace drumator {
namespace {
constexpr double kLabelX=16.0,kGridX=266.0,kGridY=108.0;
constexpr double kCellWidth=24.0,kRowHeight=29.0;
constexpr double kGridRight=kGridX+32*kCellWidth;
constexpr double kGridBottom=kGridY+16*kRowHeight;
constexpr double kHeaderY=38.0,kPatternX=252.0,kPatternWidth=70.0;
const VSTGUI::CColor background{26,35,45,255};
const VSTGUI::CColor panel{41,53,67,255};
const VSTGUI::CColor border{73,94,114,255};
const VSTGUI::CColor on{224,137,66,255};
const VSTGUI::CColor off{67,81,96,255};
const VSTGUI::CColor letters{228,236,243,255};
const VSTGUI::CColor accent{104,175,203,255};
}
DrumatorView::DrumatorView(const VSTGUI::CRect& size,
                           Controller& controller,VSTGUI::VST3Editor& editor)
 : CView(size),controller_(controller),editor_(editor) {
 setMouseEnabled(true);
}
void DrumatorView::draw(VSTGUI::CDrawContext* c) {
 c->setFillColor(background);
 c->drawRect({0,0,1100,600},VSTGUI::kDrawFilled);
 c->setFont(VSTGUI::kNormalFont,18);
 c->setFontColor(letters);
 c->drawString("DRUMATOR   /   FUNKTIONSPROTOTYP",VSTGUI::CRect(16,8,790,32),
               VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,12);
 c->setFontColor(accent);
 c->drawString("PATTERN A-H",VSTGUI::CRect(16,47,190,67),
               VSTGUI::kLeftText);
 c->drawString(status_.c_str(),VSTGUI::CRect(16,577,1050,595),
               VSTGUI::kLeftText);
 c->setFontColor(letters);
 for(std::size_t pattern=0;pattern<8;++pattern) {
   const double x=kPatternX+pattern*kPatternWidth;
   c->setFillColor(pattern==controller_.currentPattern()?accent:panel);
   c->drawRect({x,kHeaderY,x+58,kHeaderY+30},VSTGUI::kDrawFilled);
   char label[2]={static_cast<char>('A'+pattern),0};
   c->setFontColor(pattern==controller_.currentPattern()?background:letters);
   c->drawString(label,VSTGUI::CRect(x,kHeaderY,x+58,kHeaderY+30));
 }
 c->setFillColor(panel);
 c->drawRect({880,39,937,69},VSTGUI::kDrawFilled);
 c->drawRect({946,39,1027,69},VSTGUI::kDrawFilled);
 c->setFontColor(letters);
 c->drawString("100%",VSTGUI::CRect(880,39,937,69));
 c->drawString("150%",VSTGUI::CRect(946,39,1027,69));
 c->setFont(VSTGUI::kNormalFont,12);
 for(std::size_t row=0;row<kRows;++row) {
   const double y=kGridY+row*kRowHeight;
   c->setFillColor(row%2==0?panel:background);
   c->drawRect({10,y,1070,y+kRowHeight-1},VSTGUI::kDrawFilled);
   c->setFontColor(letters);
   char number[8]{};
   std::snprintf(number,sizeof(number),"%02u",static_cast<unsigned>(row+1));
   c->drawString(number,VSTGUI::CRect(kLabelX,y,kLabelX+31,y+26),
                 VSTGUI::kLeftText);
   const auto& name=controller_.label(row);
   c->drawString(name.c_str(),VSTGUI::CRect(kLabelX+36,y,kGridX-11,y+26),
                 VSTGUI::kLeftText);
   for(std::size_t step=0;step<kSteps;++step) {
     const double x=kGridX+step*kCellWidth;
     const auto v=controller_.velocity(controller_.currentPattern(),row,step);
     c->setFillColor(v?on:off);
     c->drawRect({x+2,y+4,x+20,y+22},VSTGUI::kDrawFilled);
     if(step%4==0) {
       c->setFrameColor(border);
       c->drawRect({x+1,y+3,x+21,y+23},VSTGUI::kDrawStroked);
     }
   }
 }
 setDirty(false);
}
void DrumatorView::load(std::size_t row) {
 auto* frame=getFrame();
 if(!frame)return;
 auto* picker=VSTGUI::CNewFileSelector::create(frame,
                                            VSTGUI::CNewFileSelector::kSelectFile);
 if(!picker) {status_="DATEIDIALOG NICHT VERFUEGBAR";invalid();return;}
 picker->addFileExtension(VSTGUI::CFileExtension("PCM16 WAVE","wav","audio/wav"));
 picker->setTitle("Drumator WAV-Sample auswaehlen");
 const bool selected=picker->runModal();
 if(selected&&picker->getNumSelectedFiles()>0) {
   const auto* file=picker->getSelectedFile(0);
   if(file&&controller_.loadWav(row,file))
       status_="WAV AN PROZESSOR UEBERGEBEN";
   else
       status_="WAV NICHT GELADEN: PCM16, <=16 MiB, VERBINDUNG PRUEFEN";
 }
 picker->forget();
 invalid();
}
VSTGUI::CMouseEventResult DrumatorView::onMouseDown(
   VSTGUI::CPoint& pos,const VSTGUI::CButtonState& buttons) {
 if(!(buttons&VSTGUI::kLButton))return VSTGUI::kMouseEventNotHandled;
 const double x=pos.x,y=pos.y;
 if(y>=kHeaderY&&y<kHeaderY+30) {
   if(x>=kPatternX&&x<kPatternX+8*kPatternWidth) {
     const auto index=static_cast<std::size_t>((x-kPatternX)/kPatternWidth);
     controller_.changePattern(index);invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=880&&x<=937) {editor_.setZoomFactor(1.0);return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
   if(x>=946&&x<=1027) {editor_.setZoomFactor(1.5);return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
 }
 if(y>=kGridY&&y<kGridBottom) {
   const auto row=static_cast<std::size_t>((y-kGridY)/kRowHeight);
   if(x>=kLabelX&&x<kGridX-12) {
      load(row);return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=kGridX&&x<kGridRight) {
      const auto step=static_cast<std::size_t>((x-kGridX)/kCellWidth);
      if(!controller_.toggleStep(controller_.currentPattern(),row,step))
          status_="STEP KONNTE NICHT AN PROZESSOR GESENDET WERDEN";
      invalid();
      return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
 }
 return VSTGUI::kMouseEventNotHandled;
}
}
