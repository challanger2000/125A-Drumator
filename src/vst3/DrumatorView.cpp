#include "DrumatorView.h"
#include "DrumatorController.h"
#include "core/StepVelocity.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/idatapackage.h"
#include "vstgui/lib/vstguibase.h"
#include <cctype>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <string>

namespace drumator {
namespace {
constexpr double kLabelX=16.0,kGridX=266.0,kGridY=108.0;
constexpr double kCellWidth=24.0,kRowHeight=29.0;
constexpr double kGridRight=kGridX+32*kCellWidth;
constexpr double kMinHeight=300.0;
double contentHeight(std::size_t rows) {
 return std::max(kMinHeight,kGridY+rows*kRowHeight+64.0);
}
constexpr double kHeaderY=38.0,kPatternX=252.0,kPatternWidth=70.0;
const VSTGUI::CColor background{26,35,45,255};
const VSTGUI::CColor panel{41,53,67,255};
const VSTGUI::CColor border{73,94,114,255};
const VSTGUI::CColor on{224,137,66,255};
const VSTGUI::CColor off{67,81,96,255};
const VSTGUI::CColor letters{228,236,243,255};
const VSTGUI::CColor accent{104,175,203,255};
}
// VSTGUI 4.12 drag events contain a UTF-8 file path, not sample bytes.
// Internal Studio One browser objects not exposed as file paths are
// intentionally not treated as successfully loaded samples.
class DrumatorDropTarget final : public VSTGUI::IDropTarget,
                                 public VSTGUI::AtomicReferenceCounted {
public:
 explicit DrumatorDropTarget(DrumatorView& view) : view_(view) {}
 VSTGUI::DragOperation onDragEnter(VSTGUI::DragEventData data) override {
    return allowed(data)?VSTGUI::DragOperation::Copy:VSTGUI::DragOperation::None;
 }
 VSTGUI::DragOperation onDragMove(VSTGUI::DragEventData data) override {
    return allowed(data)?VSTGUI::DragOperation::Copy:VSTGUI::DragOperation::None;
 }
 void onDragLeave(VSTGUI::DragEventData) override {}
 bool onDrop(VSTGUI::DragEventData data) override {
    return allowed(data)&&view_.acceptFileDrop(data);
 }
private:
 bool allowed(VSTGUI::DragEventData data) const {
    if(!data.drag||data.drag->getCount()!=1||
       data.drag->getDataType(0)!=VSTGUI::IDataPackage::kFilePath)
       return false;
    auto local=data.pos;
    view_.frameToLocal(local);
    if(local.y<kGridY||local.y>=kGridY+view_.visibleRowsForHit()*kRowHeight||
       local.x<kLabelX||local.x>kGridRight)return false;
    const void* bytes=nullptr;
    VSTGUI::IDataPackage::Type type=VSTGUI::IDataPackage::kError;
    const auto size=data.drag->getData(0,bytes,type);
    if(!bytes||type!=VSTGUI::IDataPackage::kFilePath||size<5||
       size>32768||static_cast<const char*>(bytes)[size-1]!='\0')
       return false;
    std::string filename(static_cast<const char*>(bytes),size-1);
    if(filename.size()<4)return false;
    auto ext=filename.substr(filename.size()-4);
    for(auto& c:ext)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext==".wav";
 }
 DrumatorView& view_;
};
std::size_t DrumatorView::visibleRowsForHit() const noexcept {
 return controller_.visibleRows();
}
bool DrumatorView::attached(VSTGUI::CView* parent) {
 const auto result=CView::attached(parent);
 if(result)adjustEditorHeight();
 return result;
}
void DrumatorView::adjustEditorHeight() {
 const double height=contentHeight(controller_.visibleRows());
 const auto zoom=editor_.getZoomFactor();
 // The host owns its window; reject an unsupported host resize explicitly.
 editor_.requestResize({1100.0*zoom,height*zoom});
 if(auto* container=getParentView()) {
   container->setViewSize({0,0,1100,height});
 }
 setViewSize({0,0,1100,height});
 invalid();
}
DrumatorView::DrumatorView(const VSTGUI::CRect& size,
                           Controller& controller,VSTGUI::VST3Editor& editor)
 : CView(size),controller_(controller),editor_(editor) {
 setMouseEnabled(true);
 setDropTarget(VSTGUI::makeOwned<DrumatorDropTarget>(*this));
}
void DrumatorView::draw(VSTGUI::CDrawContext* c) {
 c->setFillColor(background);
 c->drawRect({0,0,1100,contentHeight(controller_.visibleRows())},VSTGUI::kDrawFilled);
 c->setFont(VSTGUI::kNormalFont,18);
 c->setFontColor(letters);
 c->drawString("DRUMATOR   /   FUNKTIONSPROTOTYP",VSTGUI::CRect(16,8,790,32),
               VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,12);
 c->setFontColor(accent);
 c->drawString("PATTERN A-H | SHIFT+STEP: VELO",VSTGUI::CRect(16,47,250,67),
               VSTGUI::kLeftText);
 const double footerY=kGridY+controller_.visibleRows()*kRowHeight+39.0;
 c->drawString(status_.c_str(),VSTGUI::CRect(16,footerY,1050,footerY+21),
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
 for(std::size_t row=0;row<controller_.visibleRows();++row) {
   const double y=kGridY+row*kRowHeight;
   c->setFillColor(row%2==0?panel:background);
   c->drawRect({10,y,1070,y+kRowHeight-1},VSTGUI::kDrawFilled);
   c->setFontColor(letters);
   char number[8]{};
   std::snprintf(number,sizeof(number),"%02u",static_cast<unsigned>(row+1));
   c->drawString(number,VSTGUI::CRect(kLabelX,y,kLabelX+31,y+26),
                 VSTGUI::kLeftText);
   const auto& name=controller_.label(row);
   c->drawString(name.c_str(),VSTGUI::CRect(kLabelX+36,y,143,y+26),
                 VSTGUI::kLeftText);
   char mixText[32]{};
   std::snprintf(mixText,sizeof(mixText),"V%u P%+d T%+d",
                 controller_.rowGain(row),controller_.rowPan(row),controller_.rowPitch(row));
   c->setFontColor(accent);
   c->drawString(mixText,VSTGUI::CRect(145,y,194,y+26),VSTGUI::kLeftText);
   // Ctrl+Shift+click on the row mix display adjusts pitch by one semitone.
   c->setFillColor(controller_.isRowMuted(row)?on:off);
   c->drawRect({195,y+3,218,y+24},VSTGUI::kDrawFilled);
   c->setFontColor(letters);
   c->drawString("M",VSTGUI::CRect(195,y+3,218,y+24));
   c->setFillColor(controller_.isRowSolo(row)?accent:off);
   c->drawRect({226,y+3,249,y+24},VSTGUI::kDrawFilled);
   c->drawString("S",VSTGUI::CRect(226,y+3,249,y+24));
   for(std::size_t step=0;step<kSteps;++step) {
     const double x=kGridX+step*kCellWidth;
     const auto v=controller_.velocity(controller_.currentPattern(),row,step);
     c->setFillColor(v>=112?on:(v>=80?VSTGUI::CColor{195,126,73,255}:(v>0?VSTGUI::CColor{122,100,83,255}:off)));
     c->drawRect({x+2,y+4,x+20,y+22},VSTGUI::kDrawFilled);
     // Five-level step display: OFF has no bar; 32/64/96/127
     // increase the inset bar height. Preserve the surrounding grid.
     if(v>0) {
       const double barHeight=3.0+13.0*static_cast<double>(v)/127.0;
       c->setFillColor(VSTGUI::CColor{244,235,213,255});
       c->drawRect({x+16,y+21-barHeight,x+18,y+21},
                   VSTGUI::kDrawFilled);
     }
     if(step%4==0) {
       c->setFrameColor(border);
       c->drawRect({x+1,y+3,x+21,y+23},VSTGUI::kDrawStroked);
     }
   }
 }
 const double buttonsY=kGridY+controller_.visibleRows()*kRowHeight+5.0;
 c->setFillColor(controller_.visibleRows()<kRows?accent:panel);
 c->drawRect({16,buttonsY,213,buttonsY+29},VSTGUI::kDrawFilled);
 c->setFontColor(background);
 c->drawString(controller_.visibleRows()<kRows?"+ INSTRUMENT HINZUFUEGEN":"MAX. 16 INSTRUMENTE",
               VSTGUI::CRect(16,buttonsY,213,buttonsY+29));
 c->setFillColor(controller_.canHideLast()?panel:background);
 c->drawRect({226,buttonsY,266,buttonsY+29},VSTGUI::kDrawFilled);
 c->setFontColor(letters);
 c->drawString("-",VSTGUI::CRect(226,buttonsY,266,buttonsY+29));
 char rowsLabel[40]{};
 std::snprintf(rowsLabel,sizeof(rowsLabel),"%u / 16 ZEILEN",
               static_cast<unsigned>(controller_.visibleRows()));
 c->drawString(rowsLabel,VSTGUI::CRect(277,buttonsY,455,buttonsY+29),
               VSTGUI::kLeftText);
 setDirty(false);
}
void DrumatorView::loadPath(std::size_t row,const std::string& path) {
 const auto outcome=controller_.loadWav(row,path);
 switch(outcome) {
 case Controller::LoadStatus::Accepted:
     status_="SAMPLE ANGENOMMEN - WIEDERGABE AB NAECHSTEM AUDIOBLOCK";break;
 case Controller::LoadStatus::InvalidRow:
     status_="FEHLER: UNGUELTIGE INSTRUMENTENZEILE";break;
 case Controller::LoadStatus::NotFound:
     status_="DATEI NICHT GEFUNDEN";break;
 case Controller::LoadStatus::TooLarge:
     status_="WAV GROESSER ALS 16 MiB - PROTOTYP-LIMIT";break;
 case Controller::LoadStatus::ReadError:
     status_="WAV KONNTE NICHT GELESEN WERDEN";break;
 case Controller::LoadStatus::Unsupported:
     status_="UNTERSTUETZT: PCM 8/16/24/32 UND FLOAT32, MONO/STEREO";break;
 case Controller::LoadStatus::NoProcessorConnection:
     status_="HOST HAT CONTROLLER UND AUDIOPROZESSOR NICHT VERBUNDEN";break;
 case Controller::LoadStatus::Rejected:
     status_="PROZESSOR HAT SAMPLE ABGELEHNT - RATE/LAST PRUEFEN";break;
 }
 invalid();
}
bool DrumatorView::acceptFileDrop(VSTGUI::DragEventData data) {
 if(!data.drag||data.drag->getCount()!=1)return false;
 auto position=data.pos;
 frameToLocal(position);
 if(position.y<kGridY||position.y>=kGridY+controller_.visibleRows()*kRowHeight)return false;
 const auto row=static_cast<std::size_t>((position.y-kGridY)/kRowHeight);
 if(row>=kRows)return false;
 const void* bytes=nullptr;
 VSTGUI::IDataPackage::Type type=VSTGUI::IDataPackage::kError;
 const auto size=data.drag->getData(0,bytes,type);
 if(!bytes||type!=VSTGUI::IDataPackage::kFilePath||
    size==0||size>32768||
    static_cast<const char*>(bytes)[size-1]!='\0')return false;
 loadPath(row,std::string(static_cast<const char*>(bytes),size-1));
 return true; // drop handled; status displays actual acceptance/rejection
}
void DrumatorView::load(std::size_t row) {
 auto* frame=getFrame();
 if(!frame)return;
 auto* picker=VSTGUI::CNewFileSelector::create(frame,
                                            VSTGUI::CNewFileSelector::kSelectFile);
 if(!picker) {status_="DATEIDIALOG NICHT VERFUEGBAR";invalid();return;}
 picker->addFileExtension(VSTGUI::CFileExtension("WAV PCM/FLOAT","wav","audio/wav"));
 picker->setTitle("125A Drumator - WAV-Sample auswaehlen");
 const bool selected=picker->runModal();
 if(selected&&picker->getNumSelectedFiles()>0) {
   const auto* file=picker->getSelectedFile(0);
   if(file)loadPath(row,file);
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
 const double actionsY=kGridY+controller_.visibleRows()*kRowHeight+5.0;
 if(y>=actionsY&&y<actionsY+29) {
   if(x>=16&&x<=213) {
     if(controller_.visibleRows()>=kRows)
       status_="MAXIMAL 16 ZEILEN";
     else if(controller_.addRow())adjustEditorHeight();
     else status_="INSTRUMENT KONNTE NICHT HINZUGEFUEGT WERDEN";
     invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=226&&x<=266) {
     if(controller_.hideLastEmptyRow())adjustEditorHeight();
     else status_="ZEILE ENTHAELT STEPS/SAMPLE - NICHT ENTFERNBAR";
     invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
 }
 if(y>=kGridY&&y<kGridY+controller_.visibleRows()*kRowHeight) {
   const auto row=static_cast<std::size_t>((y-kGridY)/kRowHeight);
   if(x>=145&&x<194) {
      const bool pitch=(buttons&VSTGUI::kShift)!=0 &&
                       (buttons&VSTGUI::kControl)!=0;
      if(pitch) {
         const int pitchDelta=(buttons&VSTGUI::kAlt)!=0?-1:1;
         if(!controller_.adjustRowPitch(row,pitchDelta))status_="PITCH NICHT UEBERNOMMEN";
         invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
      }
      const bool pan=(buttons&VSTGUI::kShift)!=0;
      const int direction=(buttons&VSTGUI::kControl)!=0?-10:10;
      if(!controller_.adjustRowMix(row,pan,direction))
         status_="VOLUME/PAN NICHT UEBERNOMMEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=195&&x<218) {
      if(!controller_.toggleRowGate(row,false))status_="MUTE NICHT UEBERNOMMEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=226&&x<249) {
      if(!controller_.toggleRowGate(row,true))status_="SOLO NICHT UEBERNOMMEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=kLabelX&&x<kGridX-12) {
      load(row);return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=kGridX&&x<kGridRight) {
      const auto step=static_cast<std::size_t>((x-kGridX)/kCellWidth);
      const bool updated=(buttons&VSTGUI::kShift)!=0
          ?controller_.cycleStepVelocity(controller_.currentPattern(),row,step)
          :controller_.toggleStep(controller_.currentPattern(),row,step);
      if(!updated)
          status_="STEP KONNTE NICHT AN PROZESSOR GESENDET WERDEN";
      invalid();
      return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
 }
 return VSTGUI::kMouseEventNotHandled;
}
}
