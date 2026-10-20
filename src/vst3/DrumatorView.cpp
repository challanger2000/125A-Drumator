#include "DrumatorView.h"
#include "DrumatorEditor.h"
#include "core/RowResizeTransaction.h"
#include "core/CompactRowViewport.h"
#include "DrumatorController.h"
#include "core/StepVelocity.h"
#include "core/VelocityLane.h"
#include "core/UiRowGeometry.h"
#include "core/UiDetailGeometry.h"
#include "core/UiRowControls.h"
#include "core/UiOverlayHitZones.h"
#include "core/DropFeedback.h"
#include "core/UiHeaderHitZones.h"
#include "core/RowMixControl.h"
#include "core/Utf8Path.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/controls/ctextedit.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/idatapackage.h"
#include "vstgui/lib/vstguibase.h"
#include <cctype>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <filesystem>
#include <system_error>

namespace drumator {
namespace {
constexpr double kLabelX=16.0,kGridX=622.0,kGridY=108.0;
constexpr double kCellWidth=19.0,kRowHeight=29.0;
constexpr double kGridRight=kGridX+32*kCellWidth;
constexpr double kMinHeight=326.0;
double contentHeight(std::size_t rows,bool velocityOpen=false) {
 return std::max(kMinHeight,UiRowGeometry::gridBottom(rows,velocityOpen)+UiDetailGeometry::footerHeight);
}
constexpr double kHeaderY=38.0,kPatternX=340.0,kPatternWidth=62.0;
const VSTGUI::CColor background{26,35,45,255};
const VSTGUI::CColor panel{41,53,67,255};
const VSTGUI::CColor border{73,94,114,255};
const VSTGUI::CColor on{224,137,66,255};
const VSTGUI::CColor off{67,81,96,255};
const VSTGUI::CColor letters{228,236,243,255};
const VSTGUI::CColor accent{104,175,203,255};

// Centralized VSTGUI button renderer: consistent visible frame, fill,
// contrast and selected/disabled states, with fixed hit regions.
void drawControl(VSTGUI::CDrawContext* ctx,const VSTGUI::CRect& rect,
                 const char* title,bool selected=false,bool enabled=true) {
 const VSTGUI::CColor face=enabled?(selected?accent:panel):background;
 ctx->setFillColor(face);
 ctx->drawRect(rect,VSTGUI::kDrawFilled);
 ctx->setFrameColor(selected?letters:border);
 ctx->drawRect(rect,VSTGUI::kDrawStroked);
 ctx->setFont(VSTGUI::kNormalFont,12);
 ctx->setFontColor(!enabled?border:(selected?background:letters));
 ctx->drawString(title,rect);
}

// A single selected-instrument panel gives large, precise controls without
// shrinking the 32-step grid or growing all 16 instrument rows.
void drawPrecisionPanel(VSTGUI::CDrawContext* c,const Controller& ctrl,
                        std::size_t row,double gridBottom) {
 const auto top=UiDetailGeometry::top(gridBottom);
 c->setFillColor(panel);
 c->drawRect({16,top,1240,top+UiDetailGeometry::panelHeight},VSTGUI::kDrawFilled);
 c->setFrameColor(border);
 c->drawRect({16,top,1240,top+UiDetailGeometry::panelHeight},VSTGUI::kDrawStroked);
 char selected[50]{};
 std::snprintf(selected,sizeof(selected),"ZEILE %02u | KLANG",static_cast<unsigned>(row+1));
 c->setFont(VSTGUI::kNormalFont,13);
 c->setFontColor(letters);
 c->drawString(selected,VSTGUI::CRect(23,top+5,223,top+58),VSTGUI::kLeftText);
 const char* const labels[]={"LAUTSTAERKEIT","PANORAMA","PITCH","RANDOM VEL.","CHOKE-GRUPPE"};
 const int values[]={static_cast<int>(ctrl.rowGain(row)),ctrl.rowPan(row),ctrl.rowPitch(row),
      static_cast<int>(ctrl.rowRandomVelocity(row)),static_cast<int>(ctrl.rowChoke(row))};
 const int lows[]={0,-100,-24,0,0},highs[]={200,100,24,100,4};
 for(std::size_t i=0;i<UiDetailGeometry::slots.size();++i){
   const auto slot=UiDetailGeometry::slots[i];
   c->setFont(VSTGUI::kNormalFont,11);
   c->setFontColor(accent);
   c->drawString(labels[i],VSTGUI::CRect(slot.left+3,top+3,slot.right-3,top+24));
   drawControl(c,{slot.left+5,top+27,slot.left+42,top+60},"-");
   drawControl(c,{slot.right-42,top+27,slot.right-5,top+60},"+");
   const auto l=slot.left+47.,r=slot.right-47.;
   const double fraction=static_cast<double>(values[i]-lows[i])/
                         static_cast<double>(highs[i]-lows[i]);
   c->setFillColor(off);
   c->drawRect({l,top+51,r,top+57},VSTGUI::kDrawFilled);
   c->setFillColor(accent);
   c->drawRect({l,top+51,l+(r-l)*fraction,top+57},VSTGUI::kDrawFilled);
   char value[24]{};
   if(i==0||i==3)std::snprintf(value,sizeof(value),"%d%%",values[i]);
   else if(i==1) {
     if(values[i]==0)std::snprintf(value,sizeof(value),"MITTE");
     else std::snprintf(value,sizeof(value),"%c %d",values[i]<0?'L':'R',std::abs(values[i]));
   }else if(i==2)std::snprintf(value,sizeof(value),"%+d st",values[i]);
   else std::snprintf(value,sizeof(value),"GRUPPE %d",values[i]);
   c->setFontColor(letters);
   c->drawString(value,VSTGUI::CRect(l,top+27,r,top+49));
 }
}
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
    auto local=data.pos;
    view_.frameToLocal(local);
    if(!view_.sampleDropTargetAt(local.x,local.y))return false;
    std::string path;
    return extractSamplePath(data.drag,path);
 }
 public:
 static bool extractSamplePath(VSTGUI::IDataPackage* drag,
                                     std::string& path) {
    if (!drag)
        return false;

    const auto normalizeCandidate = [](std::string candidate) -> std::string {
        while (!candidate.empty() &&
               (candidate.back() == '\0' ||
                candidate.back() == '\r' ||
                candidate.back() == '\n')) {
            candidate.pop_back();
        }

        const auto first = candidate.find_first_not_of(" \t\r\n");
        const auto last = candidate.find_last_not_of(" \t\r\n");
        if (first == std::string::npos)
            return {};
        candidate = candidate.substr(first, last - first + 1u);

        if (candidate.size() >= 2u &&
            ((candidate.front() == '"' && candidate.back() == '"') ||
             (candidate.front() == '\'' && candidate.back() == '\''))) {
            candidate = candidate.substr(1u, candidate.size() - 2u);
        }

        constexpr std::string_view filePrefix = "file:///";
        if (candidate.size() >= filePrefix.size()) {
            bool hasFilePrefix = true;
            for (std::size_t i = 0; i < filePrefix.size(); ++i) {
                if (std::tolower(static_cast<unsigned char>(candidate[i])) !=
                    filePrefix[i]) {
                    hasFilePrefix = false;
                    break;
                }
            }

            if (hasFilePrefix) {
                candidate.erase(0u, filePrefix.size());
                std::replace(candidate.begin(), candidate.end(), '/', '\\');

                // Decode the small set of URL escapes commonly produced by
                // Windows/host browser file URLs. Unknown escapes are kept.
                std::string decoded;
                decoded.reserve(candidate.size());

                const auto hexValue = [](char ch) noexcept -> int {
                    if (ch >= '0' && ch <= '9') return ch - '0';
                    if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
                    if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
                    return -1;
                };

                for (std::size_t i = 0; i < candidate.size(); ++i) {
                    if (candidate[i] == '%' && i + 2u < candidate.size()) {
                        const int hi = hexValue(candidate[i + 1u]);
                        const int lo = hexValue(candidate[i + 2u]);
                        if (hi >= 0 && lo >= 0) {
                            decoded.push_back(
                                static_cast<char>((hi << 4) | lo));
                            i += 2u;
                            continue;
                        }
                    }
                    decoded.push_back(candidate[i]);
                }

                candidate = std::move(decoded);
            }
        }

        return candidate;
    };

    const auto isWavePath = [](const std::string& candidate) {
        if (candidate.empty())
            return false;

        std::string lower = candidate;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });

        if (lower.size() < 5u ||
            (lower.compare(lower.size() - 4u, 4u, ".wav") != 0 && lower.compare(lower.size() - 5u, 5u, ".flac") != 0)) {
            return false;
        }

        std::error_code ec;
        const auto fsPath = pathFromUtf8(candidate);
        return std::filesystem::is_regular_file(fsPath, ec) && !ec;
    };

    const auto count = drag->getCount();
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto advertisedType = drag->getDataType(i);
        if (advertisedType != VSTGUI::IDataPackage::kFilePath &&
            advertisedType != VSTGUI::IDataPackage::kText) {
            continue;
        }

        const void* buffer = nullptr;
        VSTGUI::IDataPackage::Type type {};
        const auto size = drag->getData(i, buffer, type);
        if (size == 0 || buffer == nullptr)
            continue;

        if (type != VSTGUI::IDataPackage::kFilePath &&
            type != VSTGUI::IDataPackage::kText) {
            continue;
        }

        std::string raw(
            reinterpret_cast<const char*>(buffer),
            static_cast<std::size_t>(size));

        // Some hosts expose CF_UNICODETEXT before CF_HDROP. VSTGUI maps that
        // to kText, so consider each text line as a possible file path instead
        // of rejecting the package solely because it is not kFilePath.
        std::size_t start = 0u;
        while (start <= raw.size()) {
            const auto end = raw.find_first_of("\r\n", start);
            auto candidate = normalizeCandidate(
                raw.substr(start,
                           end == std::string::npos
                               ? std::string::npos
                               : end - start));

            if (isWavePath(candidate)) {
                path = std::move(candidate);
                return true;
            }

            if (end == std::string::npos)
                break;

            start = end + 1u;
            while (start < raw.size() &&
                   (raw[start] == '\r' || raw[start] == '\n')) {
                ++start;
            }
        }
    }

    return false;
}
 DrumatorView& view_;
};
VSTGUI::DragOperation DrumatorView::onDragEnter(VSTGUI::DragEventData data) {
 std::string path;
 const bool allowed=sampleDropTargetAt(data.pos.x,data.pos.y);
 const bool hasPath=allowed&&DrumatorDropTarget::extractSamplePath(data.drag,path);
 switch(classifyDrop(allowed,hasPath)) {
   case DropFeedback::TargetBlocked:
     status_="D&D: VERDECKTES ODER UNGUELTIGES SAMPLE-ZIEL";break;
   case DropFeedback::InvalidFilePath:
     status_="D&D: KEIN GUELTIGER WAV/FLAC-DATEIPFAD";break;
   case DropFeedback::ValidTarget:
     status_="D&D: DATEIPFAD ERKANNT - AUF INSTRUMENTENZEILE ZIEHEN";break;
 }
 invalid();
 return allowed&&hasPath?VSTGUI::DragOperation::Copy:VSTGUI::DragOperation::None;
}
VSTGUI::DragOperation DrumatorView::onDragMove(VSTGUI::DragEventData data) {
 std::string path;
 return sampleDropTargetAt(data.pos.x,data.pos.y)&&
        DrumatorDropTarget::extractSamplePath(data.drag,path)
   ?VSTGUI::DragOperation::Copy:VSTGUI::DragOperation::None;
}
void DrumatorView::onDragLeave(VSTGUI::DragEventData) {}
bool DrumatorView::onDrop(VSTGUI::DragEventData data) {
 return acceptFileDrop(data);
}
std::size_t DrumatorView::visibleRowsForHit() const noexcept {
 return displayedRows();
}
bool DrumatorView::sampleDropTargetAt(double x,double y) const noexcept {
 const bool fill=fillMenuRow_>=0;
 const double top=fill?std::max(0.0,std::min(
    UiRowGeometry::rowTop(static_cast<std::size_t>(std::max(0,fillMenuRow_-static_cast<int>(firstRow_))),velocityLaneOpen_),
    getViewSize().getHeight()-230.0)):0.0;
 const auto count=displayedRows();
 const auto index=UiRowGeometry::rowAt(y,count,velocityLaneOpen_);
 return !songEditorOpen_&&x>=16.0&&x<=1230.0&&index<count&&
    UiRowGeometry::mainHit(y,index,velocityLaneOpen_)&&
    !(fill&&x>=650.0&&x<819.0&&y>=top&&y<top+225.0);
}
std::size_t DrumatorView::displayedRows() const noexcept {
 const auto total=controller_.visibleRows();
 return compactViewport_?(firstRow_<total?std::min(compactPageRows_,total-firstRow_):0):total;
}
DrumatorView::~DrumatorView() {
 controller_.unregisterView(this);
}
bool DrumatorView::removed(VSTGUI::CView* parent) {
 if(laneDragActive_){controller_.endVelocityStroke(false);laneDragActive_=false;}
 detailDragControl_=-1;detailDragRow_=kRows;
 if(nameEdit_){
   // No pending edit may commit while a native Win32 text HWND is closing.
   editingNameRow_=kRows;
   nameEditGeneration_=0;
   if(auto* frame=getFrame();frame&&frame->getFocusView()==nameEdit_)
     frame->setFocusView(nullptr);
   if(nameEdit_->getPlatformTextEdit())nameEdit_->looseFocus();
   nameEdit_->unregisterTextEditListener(this);
   if(auto* frame=getFrame())frame->removeView(nameEdit_);
   nameEdit_=nullptr;
   editingNameRow_=kRows;
 }
 return CView::removed(parent);
}
void DrumatorView::editName(std::size_t row) {
 if(row>=controller_.visibleRows()||!getFrame())return;
 const double y=UiRowGeometry::rowTop(row-firstRow_,velocityLaneOpen_);
 if(!nameEdit_){
   nameEdit_=new VSTGUI::CTextEdit({UiRowControls::sample.left,y+2,UiRowControls::sample.right,y+26},this,1001,controller_.label(row).c_str());
   nameEdit_->registerTextEditListener(this);
   getFrame()->addView(nameEdit_);
 }
 editingNameRow_=row;
 nameEditGeneration_=controller_.editGeneration();
 nameEdit_->setViewSize({UiRowControls::sample.left,y+2,UiRowControls::sample.right,y+26});
 nameEdit_->setText(controller_.customName(row).empty()?controller_.label(row).c_str():
                   controller_.customName(row).c_str());
 nameEdit_->setVisible(true);
 nameEdit_->takeFocus();
}
void DrumatorView::valueChanged(VSTGUI::CControl* control) {
 if(control!=nameEdit_||editingNameRow_>=kRows||
    nameEditGeneration_!=controller_.editGeneration())return;
 const auto text=std::string(nameEdit_->getText().getString());
 if(!controller_.renameRow(editingNameRow_,text))status_="INSTRUMENTENNAME NICHT UEBERNOMMEN";
 else status_="INSTRUMENTENNAME GESPEICHERT";
 nameEdit_->setVisible(false);
 editingNameRow_=kRows;
 nameEditGeneration_=0;
 invalid();
}
bool DrumatorView::changeDetailValue(std::size_t row,int slot,int requested) {
 if(row>=controller_.visibleRows())return false;
 const int current=slot==0?static_cast<int>(controller_.rowGain(row)):
     slot==1?controller_.rowPan(row):slot==2?controller_.rowPitch(row):
     slot==3?static_cast<int>(controller_.rowRandomVelocity(row)):
     slot==4?static_cast<int>(controller_.rowChoke(row)):-1;
 if(current==requested)return true;
 switch(slot) {
   case 0:return controller_.adjustRowMix(row,false,requested-
                               static_cast<int>(controller_.rowGain(row)));
   case 1:return controller_.adjustRowMix(row,true,requested-controller_.rowPan(row));
   case 2:return controller_.adjustRowPitch(row,requested-controller_.rowPitch(row));
   case 3:return controller_.adjustRandomVelocity(row,requested-
                               static_cast<int>(controller_.rowRandomVelocity(row)));
   case 4: {
     for(int i=0;i<4;++i) {
       const int currentChoke=static_cast<int>(controller_.rowChoke(row));
       if(currentChoke==requested)return true;
       if(!controller_.cycleRowChoke(row,requested>currentChoke?1:-1))return false;
     }
     return controller_.rowChoke(row)==static_cast<unsigned>(requested);
   }
   default:return false;
 }
}
void DrumatorView::onTextEditPlatformControlLostFocus(VSTGUI::CTextEdit* edit) {
 if(edit!=nameEdit_)return;
 nameEdit_->setVisible(false);
 editingNameRow_=kRows;
 nameEditGeneration_=0;
 invalid();
}
void DrumatorView::discardTransientEditsForRecall() noexcept {
 // Nothrow logical invalidation before the new kit commit. Do not call
 // VSTGUI/Win32 here: closing an EDIT HWND may invoke allocating callbacks.
 laneDragActive_=false;
 laneDragGeneration_=0;
 detailDragControl_=-1;
 detailDragRow_=kRows;
 editingNameRow_=kRows;
 nameEditGeneration_=0;
 fillMenuRow_=-1;
}
void DrumatorView::finishTransientEditsAfterRecall() {
 if(nameEdit_) {
   // An invisible CTextEdit can still own a visible native EDIT HWND.
   // Losing the frame focus closes the platform control via VSTGUI.
   if(auto* frame=getFrame();frame&&frame->getFocusView()==nameEdit_)
     frame->setFocusView(nullptr);
   // The text edit may retain a platform HWND even without frame focus.
   if(nameEdit_->getPlatformTextEdit())nameEdit_->looseFocus();
   nameEdit_->setVisible(false);
 }
 invalid();
}
double DrumatorView::currentZoom() const {
 return static_cast<const DrumatorEditor&>(editor_).getZoomFactor();
}
bool DrumatorView::zoomMatches(double factor) const {
 auto& editor=static_cast<DrumatorEditor&>(editor_);
 if(std::abs(editor.getZoomFactor()-factor)>0.0001)return false;
 const auto* frame=editor.getFrame();
 return !frame||std::abs(frame->getTransform().m11-editor.effectiveScale())<=0.001;
}
bool DrumatorView::applyZoomFromState(double factor) {
 auto& editor=static_cast<DrumatorEditor&>(editor_);
 if(zoomMatches(factor))return true;
 // VSTGUI::setZoomFactor is a no-op on an equal cached zoom even when the
 // native frame transform has diverged. Report the problem, never fake PASS.
 if(std::abs(editor.getZoomFactor()-factor)<0.0001)return false;
 // The checked SDK zoom already negotiates the host's frame resize. A
 // second adjustEditorHeight() request caused an unrollbackable partial zoom.
 if(!editor.setZoomChecked(factor))return false;
 invalid();
 return zoomMatches(factor);
}
void DrumatorView::refreshFromHost(bool relayout) {
 if(controller_.visibleRows()>0)
   velocityRow_=std::min(velocityRow_,controller_.visibleRows()-1);
 if(controller_.visibleRows()>0&&firstRow_>=controller_.visibleRows())firstRow_=0;
 if(relayout&&getParentView())adjustEditorHeight();
 else invalid();
}
bool DrumatorView::attached(VSTGUI::CView* parent) {
 const auto result=CView::attached(parent);
 if(result)adjustEditorHeight();
 return result;
}
bool DrumatorView::adjustEditorHeight() {
 const auto total=controller_.visibleRows();
 const auto page=velocityLaneOpen_?std::min(std::size_t{8},total):total;
 const double height=contentHeight(page,velocityLaneOpen_);
 // The host owns its window; reject an unsupported host resize explicitly.
 // The editor combines user zoom with host content scaling.
 const auto effectiveScale=static_cast<const DrumatorEditor&>(editor_).effectiveScale();
 if(!editor_.requestResize({1280.0*effectiveScale,height*effectiveScale})) {
   compactViewport_=true;
   const auto clientHeight=getViewSize().getHeight();
   auto fit=CompactRowViewport::fittingRows(clientHeight,kGridY,
       UiRowGeometry::pitch(velocityLaneOpen_),UiDetailGeometry::footerHeight);
   if(velocityLaneOpen_)fit=std::min(std::size_t{8},fit);
   // A host smaller than one complete row+footer is outside our minimum;
   // keep first row accessible instead of losing every control.
   compactPageRows_=std::max(std::size_t{1},fit);
   firstRow_=CompactRowViewport::first(controller_.visibleRows(),firstRow_);
   fillMenuRow_=-1;
   status_="HOST-RESIZE ABGELEHNT: ZEILEN MIT < > WECHSELN";
   invalid();return false;
 }
 compactViewport_=page<total;compactPageRows_=page;
 if(!compactViewport_)firstRow_=0;
 else firstRow_=std::min(firstRow_,total-page);
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
 // The view itself owns the drop-target interface, as in Phraseator.
 controller_.registerView(this);
}
void DrumatorView::draw(VSTGUI::CDrawContext* c) {
 c->setFillColor(background);
 c->drawRect({0,0,1280,contentHeight(displayedRows(),velocityLaneOpen_)},VSTGUI::kDrawFilled);
 c->setFont(VSTGUI::kNormalFont,18);
 c->setFontColor(letters);
 c->drawString("125A  /  DRUMATOR",VSTGUI::CRect(16,8,323,32),
               VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,12);
 c->setFontColor(accent);
 c->drawString("P: VORHOEREN  |  SHIFT: EDIT",VSTGUI::CRect(16,47,320,67),
               VSTGUI::kLeftText);
 const double gridBottom=UiRowGeometry::gridBottom(displayedRows(),velocityLaneOpen_);
 const double statusY=gridBottom+144.0;
 c->drawString(status_.c_str(),VSTGUI::CRect(16,statusY,1240,statusY+21),
               VSTGUI::kLeftText);
 c->setFontColor(letters);
 for(std::size_t pattern=0;pattern<8;++pattern) {
   const double x=kPatternX+pattern*kPatternWidth;
   char label[12]{};
   std::snprintf(label,sizeof(label),"%c %uT",static_cast<char>('A'+pattern),
                 controller_.patternLength(pattern)==16?1u:2u);
   drawControl(c,{x,kHeaderY,x+54,kHeaderY+30},label,
               pattern==controller_.currentPattern());
 }
 if(compactViewport_){
   c->setFontColor(accent);
   char page[64]{};
   std::snprintf(page,sizeof(page),"ZEILEN %u-%u / %u",static_cast<unsigned>(firstRow_+1),
                 static_cast<unsigned>(firstRow_+displayedRows()),static_cast<unsigned>(controller_.visibleRows()));
   c->drawString(page,VSTGUI::CRect(838,39,1000,69));
   c->drawString("<",VSTGUI::CRect(1005,39,1030,69));
   c->drawString(">",VSTGUI::CRect(1033,39,1060,69));
 }
 const double zoom=editor_.getZoomFactor();
 drawControl(c,{1070,9,1127,34},"120%",std::abs(zoom-1.2)<0.001);
 drawControl(c,{1136,9,1217,34},"130%",std::abs(zoom-1.3)<0.001);
 drawControl(c,{1070,39,1127,69},"100%",std::abs(zoom-1.0)<0.001);
 drawControl(c,{1136,39,1217,69},"150%",std::abs(zoom-1.5)<0.001);
 char swingLabel[28]{};
 std::snprintf(swingLabel,sizeof(swingLabel),"SWING %u%%",controller_.swingPercent());
 c->setFontColor(accent);
 c->drawString(swingLabel,VSTGUI::CRect(16,74,182,98),VSTGUI::kLeftText);
 drawControl(c,{185,74,214,98},"-");
 drawControl(c,{214,74,244,98},"+");
 char humanLabel[32]{};
 std::snprintf(humanLabel,sizeof(humanLabel),"HUMAN %u%%",controller_.humanizePercent());
 c->setFontColor(accent);
 c->drawString(humanLabel,VSTGUI::CRect(275,74,447,98),VSTGUI::kLeftText);
 drawControl(c,{448,74,477,98},"-");
 drawControl(c,{477,74,505,98},"+");
 drawControl(c,{540,74,638,98},controller_.songEnabled()?"SONG ON":"SONG OFF",
             controller_.songEnabled());
 drawControl(c,{652,74,850,98},songEditorOpen_?"ZURUECK ZUM PATTERN":"SONG-KETTE EDITIEREN",
             songEditorOpen_);
 char masterLabel[32]{};
 std::snprintf(masterLabel,sizeof(masterLabel),"MASTER %u%%",controller_.masterVolume());
 c->setFontColor(accent);
 c->drawString(masterLabel,VSTGUI::CRect(1037,74,1135,98),VSTGUI::kLeftText);
 drawControl(c,{1140,74,1180,98},"-");
 drawControl(c,{1184,74,1228,98},"+");
 char songSummary[64]{};
 std::snprintf(songSummary,sizeof(songSummary),"KETTE: %u / 64",
               static_cast<unsigned>(controller_.songCount()));
 c->setFontColor(accent);
 c->drawString(songSummary,VSTGUI::CRect(866,74,1030,98),VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,12);
 for(std::size_t row=firstRow_;row<firstRow_+displayedRows();++row) {
   const double y=UiRowGeometry::rowTop(row-firstRow_,velocityLaneOpen_);
   c->setFillColor(row%2==0?panel:background);
   c->drawRect({10,y,1246,y+kRowHeight-1},VSTGUI::kDrawFilled);
   if(row==velocityRow_){c->setFrameColor(accent);c->drawRect({10,y,1246,y+kRowHeight-1},VSTGUI::kDrawStroked);}
   c->setFontColor(letters);
   // Every painted x-range is the same UiRowControls slot used by hit tests.
   auto rr=[&](UiRowControls::Slot slot,double t=2.,double b=26.) {
     return VSTGUI::CRect{slot.left,y+t,slot.right,y+b};
   };
   drawControl(c,rr(UiRowControls::fill),"FILL");
   drawControl(c,rr(UiRowControls::preview),"P");
   const auto& name=controller_.label(row);
   std::string visibleName=name;
   c->setFont(VSTGUI::kNormalFont,11);
   // Fit to the actual glyph widths, not an arbitrary number of bytes.
   // UTF-8 codepoints remain whole and the WAV/FLAC extension survives.
   const double available=UiRowControls::sample.right-UiRowControls::sample.left-6.0;
   if(c->getStringWidth(visibleName.c_str())>available) {
     const auto dot=name.find_last_of('.');
     const std::string suffix=(dot!=std::string::npos&&
          name.size()-dot<=7)?name.substr(dot):std::string{};
     auto endPrefix=name.size()-suffix.size();
     bool fitted=false;
     while(endPrefix>0) {
       --endPrefix;
       while(endPrefix>0&&
            (static_cast<unsigned char>(name[endPrefix])&0xC0u)==0x80u)
         --endPrefix;
       const std::string candidate=name.substr(0,endPrefix)+"..."+suffix;
       if(c->getStringWidth(candidate.c_str())<=available) {
         visibleName=candidate;fitted=true;break;
       }
     }
     if(!fitted) {
       visibleName="..."+suffix;
       if(c->getStringWidth(visibleName.c_str())>available)
         visibleName="...";
     }
   }
   c->setFontColor(letters);
   c->drawString(visibleName.c_str(),rr(UiRowControls::sample,0,26),VSTGUI::kLeftText);
   drawControl(c,rr(UiRowControls::previous),"<");
   drawControl(c,rr(UiRowControls::next),">");
   char bus[16]{};
   if(controller_.rowOutput(row)==0)std::snprintf(bus,sizeof(bus),"MAIN");
   else std::snprintf(bus,sizeof(bus),"O%02u",controller_.rowOutput(row)+1);
   drawControl(c,rr(UiRowControls::output),bus);
   char value[32]{};
   const auto panValue=controller_.rowPan(row);
   const auto renderMix=[&](UiRowControls::Slot slot,const char* txt) {
     c->setFillColor(background);
     c->drawRect(rr(slot,1,28),VSTGUI::kDrawFilled);
     c->setFrameColor(border);
     c->drawRect(rr(slot,1,28),VSTGUI::kDrawStroked);
     c->setFont(VSTGUI::kNormalFont,11);
     c->setFontColor(accent);
     c->drawString(txt,rr(slot,1,18));
     c->setFontColor(letters);
     c->drawString("-",VSTGUI::CRect{slot.left,y+16,UiRowControls::mid(slot),y+28});
     c->drawString("+",VSTGUI::CRect{UiRowControls::mid(slot),y+16,slot.right,y+28});
   };
   std::snprintf(value,sizeof(value),"VOL %u%%",controller_.rowGain(row));
   renderMix(UiRowControls::volume,value);
   if(panValue==0)std::snprintf(value,sizeof(value),"PAN C");
   else std::snprintf(value,sizeof(value),"PAN %c%d",panValue<0?'L':'R',std::abs(panValue));
   renderMix(UiRowControls::pan,value);
   std::snprintf(value,sizeof(value),"P %+dst",controller_.rowPitch(row));
   renderMix(UiRowControls::pitch,value);
   drawControl(c,rr(UiRowControls::mute,3,24),"M",controller_.isRowMuted(row));
   drawControl(c,rr(UiRowControls::solo,3,24),"S",controller_.isRowSolo(row));
   c->setFont(VSTGUI::kNormalFont,12);
   for(std::size_t step=0;step<kSteps;++step) {
     const double x=kGridX+step*kCellWidth;
     const auto v=controller_.velocity(controller_.currentPattern(),row,step);
     c->setFillColor(v>=112?on:(v>=80?VSTGUI::CColor{195,126,73,255}:(v>0?VSTGUI::CColor{122,100,83,255}:off)));
     c->drawRect({x+2,y+4,x+17,y+22},VSTGUI::kDrawFilled);
     // Five-level step display: OFF has no bar; 32/64/96/127
     // increase the inset bar height. Preserve the surrounding grid.
     if(v>0) {
       const double barHeight=3.0+13.0*static_cast<double>(v)/127.0;
       c->setFillColor(VSTGUI::CColor{244,235,213,255});
       c->drawRect({x+13,y+21-barHeight,x+15,y+21},
                   VSTGUI::kDrawFilled);
     }
     if(step%4==0||step==16) {
       c->setFrameColor(border);
       c->drawRect({x+1,y+3,x+18,y+23},VSTGUI::kDrawStroked);
     }
   }
   if(velocityLaneOpen_) {
     const auto length=controller_.patternLength(controller_.currentPattern());
     c->setFillColor(background);
     c->drawRect({10,y+29,1246,y+55},VSTGUI::kDrawFilled);
     c->setFontColor(accent);
     c->setFont(VSTGUI::kNormalFont,11);
     char rowInfo[48]{};
     std::snprintf(rowInfo,sizeof(rowInfo),"VELOCITY  |  ZEILE %u",
         static_cast<unsigned>(row+1));
     c->drawString(rowInfo,VSTGUI::CRect(17,y+30,267,y+55),VSTGUI::kLeftText);
     if(row==velocityRow_) {
       char info[48]{};
       std::snprintf(info,sizeof(info),"STEP %u: %u / 127",
         static_cast<unsigned>(velocityStep_+1),
         static_cast<unsigned>(controller_.velocity(controller_.currentPattern(),row,velocityStep_)));
       c->drawString(info,VSTGUI::CRect(290,y+30,608,y+55),VSTGUI::kLeftText);
     }
     for(std::size_t step=0;step<kSteps;++step) {
       const double x=kGridX+step*kCellWidth;
       const auto velocity=controller_.velocity(controller_.currentPattern(),row,step);
       const bool inactive=step>=length;
       c->setFillColor(inactive?off:panel);
       c->drawRect({x+2,y+31,x+17,y+54},VSTGUI::kDrawFilled);
       if(velocity>0) {
         const double height=21.0*static_cast<double>(velocity)/127.0;
         c->setFillColor(inactive?border:accent);
         c->drawRect({x+3,y+53-height,x+16,y+53},VSTGUI::kDrawFilled);
       }
       c->setFrameColor(row==velocityRow_&&step==velocityStep_?letters:border);
       if(step%4==0||step==16||(row==velocityRow_&&step==velocityStep_))
         c->drawRect({x+1,y+30,x+18,y+55},VSTGUI::kDrawStroked);
     }
   }
 }
 const double buttonsY=gridBottom+5.0;
 c->setFillColor(controller_.visibleRows()<kRows?accent:panel);
 c->drawRect({16,buttonsY,213,buttonsY+29},VSTGUI::kDrawFilled);
 c->setFontColor(controller_.visibleRows()<kRows?background:VSTGUI::CColor{175,190,201,255});
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
 const char* actions[]={"COPY","PASTE","CLEAR","UNDO"};
 for(int i=0;i<4;++i) {
   const double x=480+i*90.;
   drawControl(c,{x,buttonsY,x+85,buttonsY+29},actions[i],
               false,i!=3||controller_.canUndoCurrentPattern());
 }
 drawControl(c,{844,buttonsY,1014,buttonsY+29},"VELOCITY VARIIEREN");
 drawControl(c,{1024,buttonsY,1220,buttonsY+29},"VELOCITY RESET 100");
 const auto selectedVelocity=controller_.velocity(controller_.currentPattern(),velocityRow_,velocityStep_);
 char velocityInfo[90]{};
 std::snprintf(velocityInfo,sizeof(velocityInfo),"STEP-VELOCITY  ZEILE %u / STEP %u : %u",
      static_cast<unsigned>(velocityRow_+1),static_cast<unsigned>(velocityStep_+1),
      static_cast<unsigned>(selectedVelocity));
 c->setFontColor(accent);
 c->drawString(velocityInfo,VSTGUI::CRect(480,buttonsY+34,915,buttonsY+64),VSTGUI::kLeftText);
 drawControl(c,{916,buttonsY+34,986,buttonsY+64},"-10");
 drawControl(c,{994,buttonsY+34,1064,buttonsY+64},"-1");
 drawControl(c,{1072,buttonsY+34,1142,buttonsY+64},"+1");
 drawControl(c,{1150,buttonsY+34,1220,buttonsY+64},"+10");
 // Row selection and explicit show/hide; no extra overlay on the steps.
 drawControl(c,{16,buttonsY+34,266,buttonsY+64},
             velocityLaneOpen_?"VELOCITY AUSBLENDEN":"VELOCITY EINBLENDEN",
             velocityLaneOpen_);
 drawControl(c,{280,buttonsY+34,324,buttonsY+64},"<");
 char laneRowLabel[32]{};
 const auto selectedRow=std::min(velocityRow_,controller_.visibleRows()-1);
 std::snprintf(laneRowLabel,sizeof(laneRowLabel),"ZEILE %02u",
               static_cast<unsigned>(selectedRow+1));
 drawControl(c,{328,buttonsY+34,404,buttonsY+64},laneRowLabel,
             velocityLaneOpen_);
 drawControl(c,{408,buttonsY+34,455,buttonsY+64},">");
 drawPrecisionPanel(c,controller_,selectedRow,gridBottom);
 if(fillMenuRow_>=0&&static_cast<std::size_t>(fillMenuRow_)<controller_.visibleRows()) {
    const double top=std::max(0.0,std::min(UiRowGeometry::rowTop(static_cast<std::size_t>(std::max(0,fillMenuRow_-static_cast<int>(firstRow_))),velocityLaneOpen_),getViewSize().getHeight()-230.0));
    const char* names[]={"FILL 2","FILL 4","FILL 8","SNARE 2+4","FILL ALL","CLEAR","OFFBEAT HIHAT","GALOPP","SNARE + GHOST"};
    for(int i=0;i<9;++i){const double y=top+i*25.;drawControl(c,{650,y,819,y+24},names[i]);}
 }
 // Full 64-slot chain board. Pattern and length choices are direct buttons.
 // Board overlays the step matrix, leaving drag-and-drop behavior untouched
 // in normal PATTERN view.
 if(songEditorOpen_) {
   c->setFillColor(background);
   c->drawRect({8,101,1252,contentHeight(displayedRows(),velocityLaneOpen_)-1},VSTGUI::kDrawFilled);
   c->setFont(VSTGUI::kNormalFont,13);
   c->setFontColor(letters);
   c->drawString("SONG-ABLAUF  |  POSITION ANKLICKEN  |  PATTERN UND LAENGE UNTEN DIREKT WAEHLEN",
                 VSTGUI::CRect(16,105,1240,126),VSTGUI::kLeftText);
   const auto count=controller_.songCount();
   for(std::size_t index=0;index<SongChain::kCapacity;++index) {
     const double sx=16+static_cast<double>(index%16)*76.0;
     const double sy=134+static_cast<double>(index/16)*32.0;
     const bool available=index<count,selected=index==songSlot_;
     c->setFillColor(selected?accent:(available?panel:background));
     c->drawRect({sx,sy,sx+73,sy+29},VSTGUI::kDrawFilled);
     c->setFontColor(selected?background:(available?letters:border));
     char cell[32]{};
     if(available) {
       const auto e=controller_.songEntry(index);
       std::snprintf(cell,sizeof(cell),"%02u %c %s",
           static_cast<unsigned>(index+1),static_cast<char>('A'+e.pattern),
           e.steps==8?"1/2":(e.steps==16?"1":"2"));
     }else std::snprintf(cell,sizeof(cell),"%02u +",static_cast<unsigned>(index+1));
     drawControl(c,{sx,sy,sx+73,sy+29},cell,selected,available||index==count);
   }
   c->setFont(VSTGUI::kNormalFont,12);
   c->setFontColor(accent);
   c->drawString("PATTERN:",VSTGUI::CRect(16,277,120,301),VSTGUI::kLeftText);
   for(std::size_t idx=0;idx<8;++idx) {
     const auto chosen=controller_.songEntry(std::min(songSlot_,count-1)).pattern==idx;
     const double bx=115+idx*56.0;
     char letter[2]{static_cast<char>('A'+idx),0};
     drawControl(c,{bx,277,bx+50,303},letter,chosen);
   }
   c->setFontColor(accent);
   c->drawString("LAENGE:",VSTGUI::CRect(594,277,681,303),VSTGUI::kLeftText);
   const char* lenNames[]={"1/2 TAKT","1 TAKT","2 TAKTE"};
   const std::uint8_t lenSteps[]={8,16,32};
   for(int idx=0;idx<3;++idx) {
     const auto chosen=controller_.songEntry(std::min(songSlot_,count-1)).steps==lenSteps[idx];
     const double bx=678+idx*106.;
     drawControl(c,{bx,277,bx+100,303},lenNames[idx],chosen);
   }
   drawControl(c,{1012,277,1110,303},"+ EINTRAG",false,count<64);
   drawControl(c,{1118,277,1239,303},"LETZTEN LOESCHEN",false,count>1);
 }
 setDirty(false);
}
bool DrumatorView::loadPath(std::size_t row,const std::string& path) {
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
 return outcome==Controller::LoadStatus::Accepted;
}
bool DrumatorView::acceptFileDrop(VSTGUI::DragEventData data) {
 if(!data.drag) {
   status_="D&D FEHLER: KEINE DATEN VOM HOST";invalid();return false;
 }
 const auto position=data.pos;
 if(!sampleDropTargetAt(position.x,position.y)) {
   status_="D&D FEHLER: AUSSERHALB DER INSTRUMENTENZEILEN";
   invalid();return false;
 }
 const auto row=firstRow_+UiRowGeometry::rowAt(position.y,displayedRows(),velocityLaneOpen_);
 if(row>=kRows) {
   status_="D&D FEHLER: ZEILE NICHT VORHANDEN";invalid();return false;
 }
 std::string path;
 if(!DrumatorDropTarget::extractSamplePath(data.drag,path)) {
   status_="D&D FEHLER: HOST UEBERGIBT KEINEN WAV/FLAC-DATEIPFAD";
   invalid();return false;
 }
 return loadPath(row,path);
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
 if(fillMenuRow_>=0) {
   const double top=std::max(0.0,std::min(UiRowGeometry::rowTop(static_cast<std::size_t>(std::max(0,fillMenuRow_-static_cast<int>(firstRow_))),velocityLaneOpen_),getViewSize().getHeight()-230.0));
   const int row=fillMenuRow_;
   fillMenuRow_=-1;
   if(x>=650&&x<819&&y>=top&&y<top+225) {
     const int option=static_cast<int>((y-top)/25.);
     const RowFill items[]={RowFill::Every2,RowFill::Every4,RowFill::Every8,RowFill::Snare24,RowFill::All,RowFill::Clear,RowFill::Offbeat8,RowFill::Gallop,RowFill::SnareGhost};
     if(option>=0&&option<9&&!controller_.fillRow(static_cast<std::size_t>(row),items[option]))
       status_="FILL NICHT UEBERNOMMEN";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   invalid();
   return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }

 if(compactViewport_&&y>=39&&y<69&&x>=1005&&x<1060) {
   if(x<1032)firstRow_=firstRow_>=compactPageRows_?firstRow_-compactPageRows_:0;
   else if(firstRow_+compactPageRows_<controller_.visibleRows())
     firstRow_+=compactPageRows_;
   fillMenuRow_=-1;invalid();
   return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(UiHeaderHitZones::masterMinus(x,y)||UiHeaderHitZones::masterPlus(x,y)){
   const int amount=(buttons&VSTGUI::kControl)?
     100-static_cast<int>(controller_.masterVolume()):
     (UiHeaderHitZones::masterMinus(x,y)?-5:5);
   if(!controller_.adjustMasterVolume(amount))status_="MASTER LEVEL NICHT UEBERNOMMEN";
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(UiHeaderHitZones::songToggle(x,y)||UiHeaderHitZones::songEditor(x,y)) {
   bool ok=true;
   if(UiHeaderHitZones::songToggle(x,y))ok=controller_.toggleSongMode();
   else {songEditorOpen_=!songEditorOpen_;if(songEditorOpen_)adjustEditorHeight();}
   if(!ok)status_="SONG-MODUS NICHT UEBERNOMMEN";
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(songEditorOpen_&&y>=101) {
   bool ok=true;
   const auto count=controller_.songCount();
   if(y>=134&&y<262&&x>=16&&x<1232) {
     const auto col=static_cast<std::size_t>((x-16)/76.);
     const auto row=static_cast<std::size_t>((y-134)/32.);
     const auto slot=row*16+col;
     if(slot<SongChain::kCapacity) {
       if(slot>=count) {
         // Clicking the next free slot extends the chain in one action.
         if(slot==count)ok=controller_.adjustSongCount(1);
         else status_="BITTE ZUERST DEN NAECHSTEN FREIEN EINTRAG WAEHLEN";
       }
       if(slot<controller_.songCount())songSlot_=slot;
     }
   }else if(y>=277&&y<303) {
     const auto slot=std::min(songSlot_,count-1);
     const auto current=controller_.songEntry(slot);
     if(x>=115&&x<563) {
       const auto index=static_cast<std::size_t>((x-115)/56.);
       if(index<8&&x<115+index*56.+50.)ok=controller_.setSongEntry(slot,static_cast<std::uint8_t>(index),current.steps);
     }else if(x>=678&&x<996) {
       const auto index=static_cast<std::size_t>((x-678)/106.);
       if(index<3&&x<678+index*106.+100.) {
         const std::uint8_t lengths[]={8,16,32};
         ok=controller_.setSongEntry(slot,current.pattern,lengths[index]);
       }
     }else if(x>=1012&&x<1110) {
       ok=controller_.adjustSongCount(1);
       if(ok)songSlot_=controller_.songCount()-1;
     }else if(x>=1118&&x<1239) {
       if(count>1)ok=controller_.adjustSongCount(-1);
       if(songSlot_>=controller_.songCount())songSlot_=controller_.songCount()-1;
     }
   }
   if(!ok)status_="SONG-EINTRAG NICHT UEBERNOMMEN";
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
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
 if(y>=kHeaderY&&y<kHeaderY+30) {
   if(x>=kPatternX&&x<kPatternX+8*kPatternWidth) {
     const auto index=static_cast<std::size_t>((x-kPatternX)/kPatternWidth);
     if(x>=kPatternX+index*kPatternWidth+54.0)
       return VSTGUI::kMouseEventNotHandled;
     const bool ok=(buttons&VSTGUI::kShift)?
       controller_.togglePatternLength(index):controller_.changePattern(index);
     if(!ok)status_="PATTERNLAENGE NICHT UEBERNOMMEN";
     invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
 }
 if((y>=39&&y<69)||(y>=9&&y<34)) {
   double requested=0.;
   if(x>=1070&&x<1127)requested=y<34?1.2:1.0;
   else if(x>=1136&&x<1217)requested=y<34?1.3:1.5;
   if(requested>0.) {
     if(static_cast<DrumatorEditor&>(editor_).setZoomChecked(requested))
       controller_.rememberZoom(requested);
     else status_="HOST HAT ZOOM ABGELEHNT";
     invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
 }
 const double gridBottom=UiRowGeometry::gridBottom(displayedRows(),velocityLaneOpen_);
 const double actionsY=gridBottom+5.0;
 if(y>=actionsY+34&&y<actionsY+64&&x>=16&&x<266) {
   const bool previous=velocityLaneOpen_;
   const bool wasCompact=compactViewport_;
   const auto wasFirstRow=firstRow_;
   velocityLaneOpen_=!previous;
   if(!adjustEditorHeight()){
     velocityLaneOpen_=previous;
     compactViewport_=wasCompact;
     firstRow_=wasFirstRow;
     (void)adjustEditorHeight();
     status_="HOST HAT FENSTERHOEHE FUER VELOCITY ABGELEHNT";
   }else status_=velocityLaneOpen_?"VELOCITY PRO ZEILE SICHTBAR":"VELOCITY AUS";
   invalid();
   return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(y>=actionsY+34&&y<actionsY+64&&x>=280&&x<455) {
   velocityRow_=std::min(velocityRow_,controller_.visibleRows()-1);
   if(x<324) {
     if(velocityRow_>0)--velocityRow_;
   }else if(x>=408) {
     if(velocityRow_+1<controller_.visibleRows())++velocityRow_;
   }
   invalid();
   return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(velocityLaneOpen_&&!songEditorOpen_) {
   const auto index=UiRowGeometry::rowAt(y,displayedRows(),true);
   if(index<displayedRows()&&UiRowGeometry::velocityHit(x,y,index)) {
     velocityRow_=firstRow_+index;
     laneDragPattern_=controller_.currentPattern();
     controller_.beginVelocityStroke(laneDragPattern_);
     laneDragGeneration_=controller_.editGeneration();
     laneDragActive_=true;
     laneErase_=(buttons&VSTGUI::kControl)!=0;
     const auto step=static_cast<std::size_t>(UiRowGeometry::stepAt(x));
     velocityStep_=step;
     if(!controller_.setStepVelocity(laneDragPattern_,velocityRow_,step,
          UiRowGeometry::velocityAt(y,index,laneErase_)))
       status_="VELOCITY-BALKEN NICHT UEBERNOMMEN";
     invalid();
     return VSTGUI::kMouseEventHandled;
   }
 }
 const auto detailSlot=UiDetailGeometry::controlAt(x,y,gridBottom);
 if(detailSlot>=0&&!songEditorOpen_) {
   const auto index=static_cast<std::size_t>(detailSlot);
   const auto row=std::min(velocityRow_,controller_.visibleRows()-1);
   const int lows[]={0,-100,-24,0,0},highs[]={200,100,24,100,4};
   const int defaults[]={100,0,0,0,0},steps[]={10,10,1,10,1};
   const int existing=detailSlot==0?static_cast<int>(controller_.rowGain(row)):
       detailSlot==1?controller_.rowPan(row):
       detailSlot==2?controller_.rowPitch(row):
       detailSlot==3?static_cast<int>(controller_.rowRandomVelocity(row)):
           static_cast<int>(controller_.rowChoke(row));
   const auto action=UiDetailGeometry::actionAt(x,detailSlot);
   const bool reset=(buttons&VSTGUI::kControl)!=0;
   int next=reset?defaults[index]:existing;
   if(!reset&&action==UiDetailGeometry::Action::Absolute)
     next=UiDetailGeometry::absoluteAt(x,detailSlot,lows[index],highs[index]);
   else if(!reset&&action==UiDetailGeometry::Action::Increment)
     next=std::min(highs[index],existing+steps[index]);
   else if(!reset&&action==UiDetailGeometry::Action::Decrement)
     next=std::max(lows[index],existing-steps[index]);
   if(next!=existing&&!changeDetailValue(row,detailSlot,next))
     status_="KLANGREGLER: AENDERUNG VOM PROZESSOR ABGELEHNT";
   if(!reset&&action==UiDetailGeometry::Action::Absolute) {
     detailDragControl_=detailSlot;
     detailDragRow_=row;
     invalid();return VSTGUI::kMouseEventHandled;
   }
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(y>=actionsY+34&&y<actionsY+64&&x>=916&&x<1220){
   int delta=0;
   if(x<986)delta=-10;
   else if(x>=994&&x<1064)delta=-1;
   else if(x>=1072&&x<1142)delta=1;
   else if(x>=1150)delta=10;
   if(delta!=0){
     const auto original=static_cast<int>(controller_.velocity(
         controller_.currentPattern(),velocityRow_,velocityStep_));
     const auto next=static_cast<std::uint8_t>(std::clamp(original+delta,1,127));
     if(!controller_.setStepVelocity(controller_.currentPattern(),velocityRow_,velocityStep_,next))
       status_="STEP-VELOCITY NICHT UEBERNOMMEN";
   }
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(y>=actionsY&&y<actionsY+29) {
   if(x>=844&&x<1220){
     const bool ok=x<1014?controller_.varyPatternVelocity():
                            (x>=1024?controller_.resetPatternVelocity():false);
     if(!ok&&x>=1014&&x<1024)return VSTGUI::kMouseEventNotHandled;
      if(!ok)status_="VELOCITY-AKTION NICHT UEBERNOMMEN";
     else status_=x<1014?"AKTIVE STEPS: VELOCITY VARIIERT":"AKTIVE STEPS: VELOCITY 100";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=480&&x<835) {
     const int action=static_cast<int>((x-480)/90.);
     if(action>=0&&action<4&&x<480+action*90.+85) {
       const bool ok=action==0?controller_.copyCurrentPattern():
          action==1?controller_.pasteCurrentPattern():
          action==2?controller_.clearCurrentPattern():
                    controller_.undoCurrentPattern();
       status_=ok?(action==3?"LETZTE PATTERN-AKTION RUECKGAENGIG":
                     "PATTERN-AKTION ERFOLGREICH"):"PATTERN-AKTION NICHT MOEGLICH";
       invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
     }
   }
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
 if(y>=kGridY&&y<gridBottom) {
   const auto index=UiRowGeometry::rowAt(y,displayedRows(),velocityLaneOpen_);
   if(index>=displayedRows()||!UiRowGeometry::mainHit(y,index,velocityLaneOpen_))
     return VSTGUI::kMouseEventNotHandled;
   const auto row=firstRow_+index;
   if(velocityRow_!=row){velocityRow_=row;invalid();}
   if(UiRowControls::hit(UiRowControls::fill,x)) {
     fillMenuRow_=static_cast<int>(row);
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(UiRowControls::hit(UiRowControls::volume,x)||
      UiRowControls::hit(UiRowControls::pan,x)||
      UiRowControls::hit(UiRowControls::pitch,x)) {
     const int slot=UiRowControls::hit(UiRowControls::pitch,x)?2:
         UiRowControls::hit(UiRowControls::pan,x)?1:0;
     const auto zone=slot==2?UiRowControls::pitch:
         slot==1?UiRowControls::pan:UiRowControls::volume;
     const auto control=slot==2?RowControl::Pitch:
         slot==1?RowControl::Pan:RowControl::Volume;
     const int current=slot==2?controller_.rowPitch(row):
         slot==1?controller_.rowPan(row):static_cast<int>(controller_.rowGain(row));
     const int target=rowControlNext(control,current,x<UiRowControls::mid(zone)?-1:1,
                                   (buttons&VSTGUI::kControl)!=0);
     if(current!=target&&!changeDetailValue(row,slot,target))
       status_="KLANGREGLER NICHT UEBERNOMMEN";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(UiRowControls::hit(UiRowControls::mute,x)) {
     if(!controller_.toggleRowGate(row,false))status_="MUTE NICHT UEBERNOMMEN";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(UiRowControls::hit(UiRowControls::solo,x)) {
     if(!controller_.toggleRowGate(row,true))status_="SOLO NICHT UEBERNOMMEN";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(UiRowControls::hit(UiRowControls::previous,x)||
      UiRowControls::hit(UiRowControls::next,x)) {
     const auto direction=UiRowControls::hit(UiRowControls::previous,x)?-1:1;
     const auto result=controller_.stepSample(row,direction);
     status_=result==Controller::LoadStatus::Accepted?"SAMPLE GEWECHSELT":
                                                   "SAMPLEWECHSEL NICHT MOEGLICH";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(UiRowControls::hit(UiRowControls::output,x)) {
     if(!controller_.cycleRowOutput(row))status_="AUSGANG NICHT UEBERNOMMEN";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(UiRowControls::hit(UiRowControls::preview,x)) {
     status_=controller_.previewSample(row)?"SAMPLE-VORSCHAU ANGEFORDERT":
                                "VORSCHAU NICHT MOEGLICH: SAMPLE LADEN / HOST-AUDIO PRUEFEN";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(UiRowControls::hit(UiRowControls::sample,x)) {
     if(buttons&VSTGUI::kShift)editName(row);
     else if(buttons&VSTGUI::kAlt) {
       status_=controller_.clearSample(row)?"SAMPLE ENTFERNT - STEPS BLEIBEN":
                                               "SAMPLE KONNTE NICHT ENTFERNT WERDEN";
     } else load(row);
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=kGridX&&x<kGridRight) {
      const auto step=static_cast<std::size_t>((x-kGridX)/kCellWidth);
      velocityRow_=row;
      velocityStep_=step;
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
VSTGUI::CMouseEventResult DrumatorView::onMouseMoved(
    VSTGUI::CPoint& pos,const VSTGUI::CButtonState& buttons) {
 if(detailDragControl_>=0) {
   if(!(buttons&VSTGUI::kLButton)){
     detailDragControl_=-1;detailDragRow_=kRows;
     return VSTGUI::kMouseEventHandled;
   }
   const int lows[]={0,-100,-24,0,0},highs[]={200,100,24,100,4};
   const auto i=static_cast<std::size_t>(detailDragControl_);
   const int next=UiDetailGeometry::absoluteAt(pos.x,detailDragControl_,lows[i],highs[i]);
   if(!changeDetailValue(detailDragRow_,detailDragControl_,next))
     status_="KLANGREGLER: AENDERUNG VOM PROZESSOR ABGELEHNT";
   invalid();return VSTGUI::kMouseEventHandled;
 }
 if(!laneDragActive_)return VSTGUI::kMouseEventNotHandled;
 if(laneDragGeneration_!=controller_.editGeneration()) {
   laneDragActive_=false;
   laneDragGeneration_=0;
   return VSTGUI::kMouseEventHandled;
 }
 if(!(buttons&VSTGUI::kLButton)){
   laneDragActive_=false;
   controller_.endVelocityStroke(true);
   return VSTGUI::kMouseEventHandled;
 }
 const auto step=UiRowGeometry::stepAt(pos.x);
 if(step<0||!velocityLaneOpen_||songEditorOpen_||
    velocityRow_<firstRow_||velocityRow_>=firstRow_+displayedRows())
   return VSTGUI::kMouseEventHandled;
 const auto mapped=UiRowGeometry::velocityAt(pos.y,velocityRow_-firstRow_,laneErase_);
 velocityStep_=static_cast<std::size_t>(step);
 if(!controller_.setStepVelocity(laneDragPattern_,velocityRow_,
                                 velocityStep_,mapped))
   status_="VELOCITY-ZIEHEN NICHT UEBERNOMMEN";
 invalid();
 return VSTGUI::kMouseEventHandled;
}
VSTGUI::CMouseEventResult DrumatorView::onMouseUp(
    VSTGUI::CPoint&,const VSTGUI::CButtonState&) {
 if(detailDragControl_>=0) {
   detailDragControl_=-1;detailDragRow_=kRows;
   return VSTGUI::kMouseEventHandled;
 }
 if(!laneDragActive_)return VSTGUI::kMouseEventNotHandled;
 const bool fresh=laneDragGeneration_==controller_.editGeneration();
 laneDragActive_=false;
 laneDragGeneration_=0;
 if(fresh)controller_.endVelocityStroke(true);
 return VSTGUI::kMouseEventHandled;
}
VSTGUI::CMouseEventResult DrumatorView::onMouseCancel() {
 detailDragControl_=-1;detailDragRow_=kRows;
 const bool fresh=laneDragGeneration_==controller_.editGeneration();
 laneDragActive_=false;
 laneDragGeneration_=0;
 if(fresh)controller_.endVelocityStroke(false);
 return VSTGUI::kMouseEventHandled;
}
}
