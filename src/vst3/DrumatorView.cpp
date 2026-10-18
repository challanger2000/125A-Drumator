#include "DrumatorView.h"
#include "DrumatorEditor.h"
#include "core/RowResizeTransaction.h"
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
constexpr double kLabelX=16.0,kGridX=560.0,kGridY=108.0;
constexpr double kCellWidth=21.0,kRowHeight=29.0;
constexpr double kGridRight=kGridX+32*kCellWidth;
constexpr double kMinHeight=300.0;
double contentHeight(std::size_t rows) {
 return std::max(kMinHeight,kGridY+rows*kRowHeight+64.0);
}
constexpr double kHeaderY=38.0,kPatternX=340.0,kPatternWidth=62.0;
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
       size>32768)
       return false;
    std::string filename(static_cast<const char*>(bytes),size);
    if(!filename.empty()&&filename.back()=='\0')filename.pop_back();
    if(filename.find('\0')!=std::string::npos)return false;
    if(filename.size()<4)return false;
    const auto point=filename.find_last_of('.');
    if(point==std::string::npos)return false;
    auto ext=filename.substr(point);
    for(auto& c:ext)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext==".wav"||ext==".flac";
 }
 DrumatorView& view_;
};
std::size_t DrumatorView::visibleRowsForHit() const noexcept {
 return controller_.visibleRows();
}
DrumatorView::~DrumatorView() {
 controller_.unregisterView(this);
}
void DrumatorView::refreshFromHost(bool relayout) {
 if(relayout&&getParentView())adjustEditorHeight();
 else invalid();
}
bool DrumatorView::attached(VSTGUI::CView* parent) {
 const auto result=CView::attached(parent);
 if(result)adjustEditorHeight();
 return result;
}
bool DrumatorView::adjustEditorHeight() {
 const double height=contentHeight(controller_.visibleRows());
 // The host owns its window; reject an unsupported host resize explicitly.
 // The editor combines user zoom with host content scaling.
 const auto effectiveScale=static_cast<const DrumatorEditor&>(editor_).effectiveScale();
 if(!editor_.requestResize({1280.0*effectiveScale,height*effectiveScale})) {
   status_="HOST HAT FENSTERGROESSE ABGELEHNT";
   invalid();return false;
 }
 if(auto* container=getParentView()) {
   container->setViewSize({0,0,1280,height});
 }
 setViewSize({0,0,1280,height});
 invalid();
 return true;
}
DrumatorView::DrumatorView(const VSTGUI::CRect& size,
                           Controller& controller,VSTGUI::VST3Editor& editor)
 : CView(size),controller_(controller),editor_(editor) {
 setMouseEnabled(true);
 setDropTarget(VSTGUI::makeOwned<DrumatorDropTarget>(*this));
 controller_.registerView(this);
}
void DrumatorView::draw(VSTGUI::CDrawContext* c) {
 c->setFillColor(background);
 c->drawRect({0,0,1280,contentHeight(controller_.visibleRows())},VSTGUI::kDrawFilled);
 c->setFont(VSTGUI::kNormalFont,18);
 c->setFontColor(letters);
 c->drawString("125A  /  DRUMATOR",VSTGUI::CRect(16,8,1000,32),
               VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,12);
 c->setFontColor(accent);
 c->drawString("PATTERN A-H   |   SHIFT+STEP: VELOCITY",VSTGUI::CRect(16,47,320,67),
               VSTGUI::kLeftText);
 const double footerY=kGridY+controller_.visibleRows()*kRowHeight+39.0;
 c->drawString(status_.c_str(),VSTGUI::CRect(16,footerY,1240,footerY+21),
               VSTGUI::kLeftText);
 c->setFontColor(letters);
 for(std::size_t pattern=0;pattern<8;++pattern) {
   const double x=kPatternX+pattern*kPatternWidth;
   c->setFillColor(pattern==controller_.currentPattern()?accent:panel);
   c->drawRect({x,kHeaderY,x+54,kHeaderY+30},VSTGUI::kDrawFilled);
   char label[2]={static_cast<char>('A'+pattern),0};
   c->setFontColor(pattern==controller_.currentPattern()?background:letters);
   c->drawString(label,VSTGUI::CRect(x,kHeaderY,x+54,kHeaderY+30));
 }
 const bool zoom150=editor_.getZoomFactor()>1.25;
 c->setFillColor(zoom150?panel:accent);
 c->drawRect({1070,39,1127,69},VSTGUI::kDrawFilled);
 c->setFillColor(zoom150?accent:panel);
 c->drawRect({1136,39,1217,69},VSTGUI::kDrawFilled);
 c->setFontColor(zoom150?letters:background);
 c->drawString("100%",VSTGUI::CRect(1070,39,1127,69));
 c->setFontColor(zoom150?background:letters);
 c->drawString("150%",VSTGUI::CRect(1136,39,1217,69));
 char swingLabel[28]{};
 std::snprintf(swingLabel,sizeof(swingLabel),"SWING %u%%",controller_.swingPercent());
 c->setFontColor(accent);
 c->drawString(swingLabel,VSTGUI::CRect(16,74,182,98),VSTGUI::kLeftText);
 c->drawString("- / +",VSTGUI::CRect(185,74,244,98));
 char humanLabel[32]{};
 std::snprintf(humanLabel,sizeof(humanLabel),"HUMAN %u%%",controller_.humanizePercent());
 c->setFontColor(accent);
 c->drawString(humanLabel,VSTGUI::CRect(275,74,447,98),VSTGUI::kLeftText);
 c->drawString("- / +",VSTGUI::CRect(448,74,505,98));
 c->setFontColor(letters);
 c->drawString("VOL / PAN / P: LINKS -  RECHTS +   |   ALT+SAMPLE: ENTFERNEN",
               VSTGUI::CRect(545,74,1240,98),VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,12);
 for(std::size_t row=0;row<controller_.visibleRows();++row) {
   const double y=kGridY+row*kRowHeight;
   c->setFillColor(row%2==0?panel:background);
   c->drawRect({10,y,1246,y+kRowHeight-1},VSTGUI::kDrawFilled);
   c->setFontColor(letters);
   char number[8]{};
   std::snprintf(number,sizeof(number),"%02u",static_cast<unsigned>(row+1));
   c->drawString(number,VSTGUI::CRect(kLabelX,y,kLabelX+31,y+26),
                 VSTGUI::kLeftText);
   const auto& name=controller_.label(row);
   // Fit the filename in the fixed row area; preserve the extension where possible.
   std::string visibleName=name;
   if(visibleName.size()>27) {
     // Truncate on UTF-8 codepoint boundaries, never inside a character.
     auto prefixEnd=std::min<std::size_t>(17,name.size());
     while(prefixEnd>0&&prefixEnd<name.size()&&
           (static_cast<unsigned char>(name[prefixEnd])&0xC0u)==0x80u)
       --prefixEnd;
     auto suffixBegin=name.size()-7;
     while(suffixBegin<name.size()&&
           (static_cast<unsigned char>(name[suffixBegin])&0xC0u)==0x80u)
       ++suffixBegin;
     visibleName=name.substr(0,prefixEnd)+"..."+name.substr(suffixBegin);
   }
   c->drawString(visibleName.c_str(),VSTGUI::CRect(kLabelX+36,y,275,y+26),
                 VSTGUI::kLeftText);
   char value[32]{};
   c->setFontColor(accent);
   std::snprintf(value,sizeof(value),"VOL %u",controller_.rowGain(row));
   c->drawString(value,VSTGUI::CRect(280,y,345,y+26),VSTGUI::kLeftText);
   std::snprintf(value,sizeof(value),"PAN %+d",controller_.rowPan(row));
   c->drawString(value,VSTGUI::CRect(350,y,415,y+26),VSTGUI::kLeftText);
   std::snprintf(value,sizeof(value),"P %+d",controller_.rowPitch(row));
   c->drawString(value,VSTGUI::CRect(420,y,475,y+26),VSTGUI::kLeftText);
   c->setFillColor(controller_.isRowMuted(row)?on:off);
   c->drawRect({478,y+3,500,y+24},VSTGUI::kDrawFilled);
   c->setFontColor(letters);
   c->drawString("M",VSTGUI::CRect(478,y+3,500,y+24));
   c->setFillColor(controller_.isRowSolo(row)?accent:off);
   c->drawRect({507,y+3,529,y+24},VSTGUI::kDrawFilled);
   c->drawString("S",VSTGUI::CRect(507,y+3,529,y+24));
   char chokeText[8]{};
   std::snprintf(chokeText,sizeof(chokeText),"%u",controller_.rowChoke(row));
   c->setFontColor(accent);
   c->drawString(chokeText,VSTGUI::CRect(535,y+3,558,y+24));
   for(std::size_t step=0;step<kSteps;++step) {
     const double x=kGridX+step*kCellWidth;
     const auto v=controller_.velocity(controller_.currentPattern(),row,step);
     c->setFillColor(v>=112?on:(v>=80?VSTGUI::CColor{195,126,73,255}:(v>0?VSTGUI::CColor{122,100,83,255}:off)));
     c->drawRect({x+2,y+4,x+18,y+22},VSTGUI::kDrawFilled);
     // Five-level step display: OFF has no bar; 32/64/96/127
     // increase the inset bar height. Preserve the surrounding grid.
     if(v>0) {
       const double barHeight=3.0+13.0*static_cast<double>(v)/127.0;
       c->setFillColor(VSTGUI::CColor{244,235,213,255});
       c->drawRect({x+14,y+21-barHeight,x+16,y+21},
                   VSTGUI::kDrawFilled);
     }
     if(step%4==0) {
       c->setFrameColor(border);
       c->drawRect({x+1,y+3,x+19,y+23},VSTGUI::kDrawStroked);
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
     status_="SAMPLE ZU GROSS (MAX. 16 MiB)";break;
 case Controller::LoadStatus::ReadError:
     status_="SAMPLE KONNTE NICHT GELESEN WERDEN";break;
 case Controller::LoadStatus::Unsupported:
     status_="UNTERSTUETZT: WAV PCM/FLOAT UND FLAC, MONO/STEREO";break;
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
    size==0||size>32768)return false;
 std::string path(static_cast<const char*>(bytes),size);
 if(!path.empty()&&path.back()=='\0')path.pop_back();
 if(path.empty()||path.find('\0')!=std::string::npos)return false;
 loadPath(row,path);
 return true; // drop handled; status displays actual acceptance/rejection
}
void DrumatorView::load(std::size_t row) {
 auto* frame=getFrame();
 if(!frame)return;
 auto* picker=VSTGUI::CNewFileSelector::create(frame,
                                            VSTGUI::CNewFileSelector::kSelectFile);
 if(!picker) {status_="DATEIDIALOG NICHT VERFUEGBAR";invalid();return;}
 picker->addFileExtension(VSTGUI::CFileExtension("WAV PCM/FLOAT","wav","audio/wav"));
 picker->addFileExtension(VSTGUI::CFileExtension("FLAC","flac","audio/flac"));
 picker->setTitle("125A Drumator - WAV/FLAC-Sample auswaehlen");
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
 if(y>=74&&y<98&&x>=448&&x<505) {
   if(!controller_.adjustHumanize((buttons&VSTGUI::kControl)?
                                   -static_cast<int>(controller_.humanizePercent()):
                                   (x<477?-10:10)))
      status_="HUMANIZE NICHT UEBERNOMMEN";
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(y>=74&&y<98&&x>=185&&x<244) {
   if(!controller_.adjustSwing((buttons&VSTGUI::kControl)?
                                 -static_cast<int>(controller_.swingPercent()):
                                 (x<214?-5:5)))status_="SWING NICHT UEBERNOMMEN";
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(y>=kHeaderY&&y<kHeaderY+31) {
   if(x>=kPatternX&&x<kPatternX+8*kPatternWidth) {
     const auto index=static_cast<std::size_t>((x-kPatternX)/kPatternWidth);
     if(x>=kPatternX+index*kPatternWidth+54.0)
       return VSTGUI::kMouseEventNotHandled;
     controller_.changePattern(index);invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=1070&&x<=1127) {editor_.setZoomFactor(1.0);invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
   if(x>=1136&&x<=1217) {editor_.setZoomFactor(1.5);invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
 }
 const double actionsY=kGridY+controller_.visibleRows()*kRowHeight+5.0;
 if(y>=actionsY&&y<actionsY+29) {
   if(x>=16&&x<=213) {
     if(controller_.visibleRows()>=kRows)
       status_="MAXIMAL 16 ZEILEN";
     else {
       const auto previous=controller_.visibleRows();
       const auto result=tryRowResizeTransaction(
           [&](){return controller_.addRow();},
           [&](){return adjustEditorHeight();},
           [&](){return controller_.restoreVisibleRows(previous);});
       if(result==RowResizeResult::EditRejected)
         status_="INSTRUMENT KONNTE NICHT HINZUGEFUEGT WERDEN";
       else if(result==RowResizeResult::RollbackFailed)
         status_="ZEILEN-ROLLBACK FEHLGESCHLAGEN";
     }
     invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=226&&x<=266) {
     const auto previous=controller_.visibleRows();
     const auto result=tryRowResizeTransaction(
         [&](){return controller_.hideLastEmptyRow();},
         [&](){return adjustEditorHeight();},
         [&](){return controller_.restoreVisibleRows(previous);});
     if(result==RowResizeResult::EditRejected)
       status_="ZEILE ENTHAELT STEPS/SAMPLE - NICHT ENTFERNBAR";
     else if(result==RowResizeResult::RollbackFailed)
       status_="ZEILEN-ROLLBACK FEHLGESCHLAGEN";
     invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
 }
 if(y>=kGridY&&y<kGridY+controller_.visibleRows()*kRowHeight) {
   const auto row=static_cast<std::size_t>((y-kGridY)/kRowHeight);
   if((x>=280&&x<345)||(x>=350&&x<415)||(x>=420&&x<475)) {
      const bool pitch=x>=420;
      const bool pan=x>=350&&x<415;
      // Direct left/right half controls, modifier-free at the default zoom.
      const double start=pitch?420.0:(pan?350.0:280.0);
      const double width=pitch?55.0:65.0;
      const int delta=x<start+width/2.0?-1:1;
      if(pitch) {
         if(!controller_.adjustRowPitch(row,(buttons&VSTGUI::kControl)?
                                            -controller_.rowPitch(row):delta))
            status_="PITCH NICHT UEBERNOMMEN";
      } else {
         const int mixDelta=(buttons&VSTGUI::kControl)?
            (pan?-controller_.rowPan(row):100-static_cast<int>(controller_.rowGain(row))):
            delta*10;
         if(!controller_.adjustRowMix(row,pan,mixDelta))
            status_="VOLUME/PAN NICHT UEBERNOMMEN";
      }
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=478&&x<500) {
      if(!controller_.toggleRowGate(row,false))status_="MUTE NICHT UEBERNOMMEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=507&&x<529) {
      if(!controller_.toggleRowGate(row,true))status_="SOLO NICHT UEBERNOMMEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=535&&x<558) {
      if(!controller_.cycleRowChoke(row))status_="CHOKE NICHT UEBERNOMMEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=kLabelX&&x<275&&(buttons&VSTGUI::kAlt)!=0) {
      if(controller_.clearSample(row))status_="SAMPLE ENTFERNT - STEPS BLEIBEN";
      else status_="SAMPLE KONNTE NICHT ENTFERNT WERDEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=kLabelX&&x<275) {
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
