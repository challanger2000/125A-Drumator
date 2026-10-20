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
#include "core/UiCompactLayout.h"
#include "core/UiSongLayout.h"
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
constexpr double kGridX=UiCompactLayout::stepX,kGridY=UiCompactLayout::header;
constexpr double kCellWidth=UiCompactLayout::stepWidth,kGridRight=kGridX+32*kCellWidth;
constexpr double kPatternX=330.,kPatternWidth=55.,kPatternY=52.;
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

void drawRotary(VSTGUI::CDrawContext* c,double cx,double cy,double fraction,
                const char* title,const char* value,bool selected=false) {
 fraction=std::clamp(fraction,0.,1.);
 c->setFont(VSTGUI::kNormalFont,11);c->setFontColor(accent);
 c->drawString(title,{cx-46.,cy-34.,cx+46.,cy-18.});
 c->setFillColor(selected?VSTGUI::CColor{58,89,109,255}:panel);
 c->drawEllipse({cx-17.,cy-17.,cx+17.,cy+17.},VSTGUI::kDrawFilled);
 c->setFrameColor(selected?letters:border);
 c->setLineWidth(selected?2.:1.3);
 c->drawEllipse({cx-17.,cy-17.,cx+17.,cy+17.},VSTGUI::kDrawStroked);
 const auto angle=(-135.+270.*fraction)*3.14159265358979323846/180.;
 c->setFrameColor(on);c->setLineWidth(2.5);
 c->drawLine({cx,cy},{cx+std::sin(angle)*13.,cy-std::cos(angle)*13.});
 c->setLineWidth(1.);c->setFont(VSTGUI::kNormalFont,12);c->setFontColor(letters);
 c->drawString(value,{cx+21.,cy-11.,cx+72.,cy+14.},VSTGUI::kLeftText);
}
} // anonymous namespace: shared rotary painter and palette
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
std::size_t DrumatorView::visibleRowsForHit() const noexcept {return displayedRows();}
std::size_t DrumatorView::displayedRows() const noexcept {
 const auto total=controller_.visibleRows();
 return firstRow_<total?std::min(compactPageRows_,total-firstRow_):0;
}
double DrumatorView::rowTopOnPage(std::size_t index) const noexcept {
 return UiCompactLayout::rowTop(firstRow_,index,velocityExpanded_);
}
double DrumatorView::gridBottom() const noexcept {return rowTopOnPage(displayedRows());}
void DrumatorView::closeTransientMenus() noexcept {
 popup_=0;popupRow_=kRows;fillMenuRow_=-1;
}
bool DrumatorView::toggleRowVelocity(std::size_t row) {
 if(row>=controller_.visibleRows())return false;
 if(laneDragActive_){controller_.endVelocityStroke(false);laneDragActive_=false;}
 const auto previous=velocityExpanded_[row];
 velocityExpanded_[row]=!previous;
 if(!adjustEditorHeight()){
   velocityExpanded_[row]=previous;
   (void)adjustEditorHeight();
   status_="HOST HAT HOEHE FUER VELOCITY ABGELEHNT";
   return false;
 }
 return true;
}
bool DrumatorView::changeKnobValue(int slot,std::size_t row,int value) {
 if(slot>=0&&slot<=3)return changeDetailValue(row,slot,value);
 if(slot==4)return controller_.adjustSwing(value-static_cast<int>(controller_.swingPercent()));
 if(slot==5)return controller_.adjustHumanize(value-static_cast<int>(controller_.humanizePercent()));
 if(slot==6)return controller_.adjustMasterVolume(value-static_cast<int>(controller_.masterVolume()));
 return false;
}
bool DrumatorView::sampleDropTargetAt(double x,double y) const noexcept {
 if(songEditorOpen_||popup_!=0||fillMenuRow_>=0)return false;
 const auto index=UiCompactLayout::rowAt(y,firstRow_,displayedRows(),velocityExpanded_);
 if(index>=displayedRows())return false;
 const auto top=rowTopOnPage(index);
 return UiCompactLayout::mainHit(y,top)&&x>=UiCompactLayout::sample.left&&
        x<UiCompactLayout::sample.right;
}
DrumatorView::~DrumatorView() {
 controller_.unregisterView(this);
}
bool DrumatorView::removed(VSTGUI::CView* parent) {
 if(laneDragActive_){controller_.endVelocityStroke(false);laneDragActive_=false;}
 detailDragControl_=-1;detailDragRow_=kRows;
 activeKnob_=-1;knobRow_=kRows;
 closeTransientMenus();
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
 const double y=rowTopOnPage(row-firstRow_);
 if(!nameEdit_){
   nameEdit_=new VSTGUI::CTextEdit({UiCompactLayout::sample.left,y+4,UiCompactLayout::sample.right,y+33},this,1001,controller_.label(row).c_str());
   nameEdit_->registerTextEditListener(this);
   getFrame()->addView(nameEdit_);
 }
 editingNameRow_=row;
 nameEditGeneration_=controller_.editGeneration();
 nameEdit_->setViewSize({UiCompactLayout::sample.left,y+4,UiCompactLayout::sample.right,y+33});
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
 activeKnob_=-1;knobRow_=kRows;knobGeneration_=0;knobFine_=false;
 closeTransientMenus();
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
 if(songSlot_>=controller_.songCount()){
   songSlot_=controller_.songCount()-1;
   songPage_=UiSongLayout::pageOf(songSlot_);
 }
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
 if(firstRow_>=total)firstRow_=total>0?((total-1)/UiCompactLayout::perPage)*UiCompactLayout::perPage:0;
 const auto page=std::min(UiCompactLayout::perPage,total-firstRow_);
 const auto height=std::max(370.,UiCompactLayout::editorHeight(firstRow_,page,velocityExpanded_));
 const auto scale=static_cast<const DrumatorEditor&>(editor_).effectiveScale();
 if(!editor_.requestResize({1280.*scale,height*scale})){
   compactViewport_=true;
   const auto available=getViewSize().getHeight();
   compactPageRows_=0;
   for(std::size_t i=1;i<=page;++i) {
     if(UiCompactLayout::editorHeight(firstRow_,i,velocityExpanded_)>available)break;
     compactPageRows_=i;
   }
   if(compactPageRows_==0)compactPageRows_=1;
   status_="HOST-FENSTER ZU KLEIN: ZEILEN MIT < > WECHSELN";
   invalid();return false;
 }
 compactPageRows_=UiCompactLayout::perPage;
 compactViewport_=total>UiCompactLayout::perPage;
 if(velocityRow_<firstRow_||velocityRow_>=firstRow_+displayedRows())
   velocityRow_=firstRow_;
 if(auto* parent=getParentView())parent->setViewSize({0,0,1280,height});
 setViewSize({0,0,1280,height});
 invalid();return true;
}
DrumatorView::DrumatorView(const VSTGUI::CRect& size,
                           Controller& controller,VSTGUI::VST3Editor& editor)
 : CView(size),controller_(controller),editor_(editor) {
 setMouseEnabled(true);
 // The view itself owns the drop-target interface, as in Phraseator.
 controller_.registerView(this);
}
void DrumatorView::draw(VSTGUI::CDrawContext* c) {
 const auto bottom=gridBottom();
 const auto fullHeight=getViewSize().getHeight();
 c->setFillColor(background);
 c->drawRect({0,0,1280,fullHeight},VSTGUI::kDrawFilled);
 c->setFont(VSTGUI::kNormalFont,20);c->setFontColor(letters);
 c->drawString("125A  DRUMATOR",{16,5,304,35},VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,11);c->setFontColor(accent);
 c->drawString("PATTERN",{330,13,460,33},VSTGUI::kLeftText);
 const bool oneBar=controller_.patternLength(controller_.currentPattern())==16;
 drawControl(c,{460,8,526,34},"1 TAKT",oneBar);
 drawControl(c,{533,8,608,34},"2 TAKTE",!oneBar);
 for(std::size_t pattern=0;pattern<8;++pattern) {
   char label[12]{};
   std::snprintf(label,sizeof(label),"%c %uT",static_cast<char>('A'+pattern),
                 controller_.patternLength(pattern)==16?1u:2u);
   const auto x=kPatternX+pattern*kPatternWidth;
   drawControl(c,{x,kPatternY,x+49,kPatternY+30},label,
               pattern==controller_.currentPattern());
 }
 drawControl(c,{782,52,901,82},"PATTERN ...",popup_==2);
 drawControl(c,{910,52,991,82},controller_.songEnabled()?"SONG ON":"SONG OFF",
             controller_.songEnabled());
 drawControl(c,{998,52,1098,82},songEditorOpen_?"PATTERN":"SONG EDIT",songEditorOpen_);
 const auto zoom=editor_.getZoomFactor();
 char zoomLabel[32]{};
 std::snprintf(zoomLabel,sizeof(zoomLabel),"ZOOM %u%%   v",
               static_cast<unsigned>(std::lround(zoom*100.)));
 drawControl(c,{1120,7,1250,35},zoomLabel,popup_==1);
 char text[36]{};
 std::snprintf(text,sizeof(text),"%u%%",controller_.swingPercent());
 drawRotary(c,80,76,controller_.swingPercent()/50.,"SWING",text,activeKnob_==4);
 std::snprintf(text,sizeof(text),"%u%%",controller_.humanizePercent());
 drawRotary(c,178,76,controller_.humanizePercent()/100.,"HUMANIZE",text,activeKnob_==5);
 std::snprintf(text,sizeof(text),"%u%%",controller_.masterVolume());
 drawRotary(c,1178,76,controller_.masterVolume()/200.,"MASTER",text,activeKnob_==6);
 // Four full-fledged rows per page: the selection has its own independent hit box.
 for(std::size_t i=0;i<displayedRows();++i) {
   const auto row=firstRow_+i;
   const double y=rowTopOnPage(i);
   const bool chosen=row==velocityRow_;
   const auto rowH=UiCompactLayout::baseRow+(velocityExpanded_[row]?UiCompactLayout::velocityRow:0.);
   c->setFillColor(chosen?VSTGUI::CColor{42,69,88,255}:(i%2?background:panel));
   c->drawRect({9,y,1270,y+rowH-2.},VSTGUI::kDrawFilled);
   c->setFrameColor(chosen?accent:border);
   c->drawRect({9,y,1270,y+rowH-2.},VSTGUI::kDrawStroked);
   char name[16]{};
   std::snprintf(name,sizeof(name),"%02u",static_cast<unsigned>(row+1));
   drawControl(c,{16,y+5,74,y+35},name,chosen);
   // Sample name remains editable on Shift-click; click opens WAV/FLAC picker.
   const auto& raw=controller_.label(row);
   std::string fitted=raw;
   c->setFont(VSTGUI::kNormalFont,12);
   const double space=UiCompactLayout::sample.right-UiCompactLayout::sample.left-8.;
   if(c->getStringWidth(fitted.c_str())>space) {
     const auto pos=raw.find_last_of('.');
     const auto suffix=(pos!=std::string::npos&&raw.size()-pos<=7)?raw.substr(pos):std::string{};
     std::size_t n=raw.size()-suffix.size();
     while(n>0) {
       --n;
       while(n>0&&(static_cast<unsigned char>(raw[n])&0xC0u)==0x80u)--n;
       fitted=raw.substr(0,n)+"..."+suffix;
       if(c->getStringWidth(fitted.c_str())<=space)break;
     }
     if(c->getStringWidth(fitted.c_str())>space)fitted="...";
   }
   c->setFillColor(background);
   c->drawRect({78,y+5,272,y+35},VSTGUI::kDrawFilled);
   c->setFrameColor(border);c->drawRect({78,y+5,272,y+35},VSTGUI::kDrawStroked);
   c->setFontColor(letters);
   c->drawString(fitted.c_str(),{83,y+5,270,y+35},VSTGUI::kLeftText);
   drawControl(c,{275,y+5,301,y+35},"...");
   drawControl(c,{303,y+5,329,y+35},"<");
   drawControl(c,{332,y+5,358,y+35},">");
   drawControl(c,{362,y+5,398,y+35},"PLAY");
   drawControl(c,{403,y+5,452,y+35},"FILL");
   drawControl(c,{457,y+5,491,y+35},"M",controller_.isRowMuted(row));
   drawControl(c,{496,y+5,530,y+35},"S",controller_.isRowSolo(row));
   const auto out=controller_.rowOutput(row);
   if(out==0)std::snprintf(name,sizeof(name),"MAIN v");
   else std::snprintf(name,sizeof(name),"OUT %u v",out+1);
   drawControl(c,{535,y+5,615,y+35},name,popup_==3&&popupRow_==row);
   std::snprintf(name,sizeof(name),"CH%u v",controller_.rowChoke(row));
   c->setFont(VSTGUI::kNormalFont,10);c->setFontColor(accent);
   c->drawString("CHOKE",{570,y+37,615,y+52});
   drawControl(c,{570,y+54,615,y+86},name,popup_==4&&popupRow_==row);
   const double centers[]={133.,254.,375.,496.};
   const int values[]={static_cast<int>(controller_.rowGain(row)),controller_.rowPan(row),
                       controller_.rowPitch(row),static_cast<int>(controller_.rowRandomVelocity(row))};
   const char* labels[]={"VOL","PAN","PITCH","RANDOM VEL."};
   for(int k=0;k<4;++k){
     const int lo=UiCompactLayout::minValue(k),hi=UiCompactLayout::maxValue(k);
     const int v=values[k];
     char desc[24]{};
     if(k==0||k==3)std::snprintf(desc,sizeof(desc),"%d%%",v);
     else if(k==1){
       if(v==0)std::snprintf(desc,sizeof(desc),"C");
       else std::snprintf(desc,sizeof(desc),"%c%d",v<0?'L':'R',std::abs(v));
     }else std::snprintf(desc,sizeof(desc),"%+d st",v);
     drawRotary(c,centers[k],y+68,
                static_cast<double>(v-lo)/static_cast<double>(hi-lo),
                labels[k],desc,chosen&&activeKnob_==k);
   }
   const auto steps=controller_.patternLength(controller_.currentPattern());
   for(std::size_t step=0;step<32;++step) {
     const auto sx=kGridX+step*kCellWidth;
     const auto velocity=controller_.velocity(controller_.currentPattern(),row,step);
     const bool inactive=step>=steps;
     if(step%4==0) {
       char index[8]{};
       std::snprintf(index,sizeof(index),"%u",static_cast<unsigned>(step+1));
       c->setFont(VSTGUI::kNormalFont,10);c->setFontColor(border);
       c->drawString(index,{sx,y+10,sx+32,y+28},VSTGUI::kLeftText);
     }
     c->setFillColor(inactive?off:velocity>=100?on:velocity>0?VSTGUI::CColor{148,119,88,255}:off);
     c->drawRect({sx+2,y+45,sx+17,y+77},VSTGUI::kDrawFilled);
     c->setFrameColor(step%4==0?letters:border);
     c->drawRect({sx+1,y+44,sx+18,y+78},VSTGUI::kDrawStroked);
     if(velocity>0) {
       c->setFillColor(letters);
       c->drawRect({sx+13,y+77.-static_cast<double>(velocity)*24./127.,sx+15,y+77.},VSTGUI::kDrawFilled);
     }
   }
   c->setFont(VSTGUI::kNormalFont,10);c->setFontColor(accent);
   c->drawString("VELO",{1233,y+12,1266,y+35});
   drawControl(c,{1233,y+45,1266,y+79},velocityExpanded_[row]?"^":"v",velocityExpanded_[row]);
   if(velocityExpanded_[row]){
     c->setFillColor(background);
     c->drawRect({16,y+94,1266,y+123},VSTGUI::kDrawFilled);
     c->setFont(VSTGUI::kNormalFont,11);c->setFontColor(accent);
     c->drawString("VELOCITY",{78,y+94,170,y+122},VSTGUI::kLeftText);
     if(row==velocityRow_) {
       char current[48]{};
       std::snprintf(current,sizeof(current),"STEP %u : %u / 127",
         static_cast<unsigned>(velocityStep_+1),
         static_cast<unsigned>(controller_.velocity(controller_.currentPattern(),row,velocityStep_)));
       c->drawString(current,{195,y+94,390,y+122},VSTGUI::kLeftText);
     }
     for(std::size_t step=0;step<32;++step){
       const double sx=kGridX+step*kCellWidth;
       const auto vel=controller_.velocity(controller_.currentPattern(),row,step);
       c->setFillColor(step>=steps?off:panel);
       c->drawRect({sx+2,y+97,sx+17,y+120},VSTGUI::kDrawFilled);
       if(vel>0) {
         c->setFillColor(step>=steps?border:accent);
         c->drawRect({sx+3,y+119.-21.*vel/127.,sx+16,y+119.},VSTGUI::kDrawFilled);
       }
     }
   }
 }
 // Only row management and one status line remain at the bottom.
 drawControl(c,{UiCompactLayout::rowAdd.left,bottom+5,UiCompactLayout::rowAdd.right,bottom+34},"+ ZEILE");
 drawControl(c,{UiCompactLayout::rowRemove.left,bottom+5,UiCompactLayout::rowRemove.right,bottom+34},"- ZEILE",false,controller_.canHideLast());
 char range[64]{};
 std::snprintf(range,sizeof(range),"%u-%u / %u INSTRUMENTE",
   static_cast<unsigned>(firstRow_+1),static_cast<unsigned>(firstRow_+displayedRows()),
   static_cast<unsigned>(controller_.visibleRows()));
 c->setFontColor(accent);c->setFont(VSTGUI::kNormalFont,12);
 c->drawString(range,{222,bottom+5,410,bottom+34},VSTGUI::kLeftText);
 c->drawString(status_.c_str(),{414,bottom+5,1016,bottom+34},VSTGUI::kLeftText);
 if(controller_.visibleRows()>UiCompactLayout::perPage) {
   drawControl(c,{1068,bottom+5,1145,bottom+34},"<");
   drawControl(c,{1152,bottom+5,1229,bottom+34},">");
 }
 if(songEditorOpen_) {
   // Dedicated, paged Song surface. Only valid entries and the next add-slot
   // are interactive; no 64-button wall or invisible selection hotspots.
   c->setFillColor(background);
   c->drawRect({0,101,1280,getViewSize().getHeight()},VSTGUI::kDrawFilled);
   const auto count=controller_.songCount();
   const auto first=UiSongLayout::first(songPage_);
   char header[128]{};
   std::snprintf(header,sizeof(header),
       "SONG-ABLAUF   %02u-%02u / 64   |   POSITION WAEHLEN, DANACH PATTERN UND LAENGE",
       static_cast<unsigned>(first+1),static_cast<unsigned>(first+UiSongLayout::perPage));
   c->setFont(VSTGUI::kNormalFont,12);c->setFontColor(letters);
   c->drawString(header,{16,105,975,133},VSTGUI::kLeftText);
   drawControl(c,{UiSongLayout::previousLeft,UiSongLayout::navTop,
                  UiSongLayout::previousRight,UiSongLayout::navBottom},
               "< ZURUECK",false,songPage_>0);
   drawControl(c,{UiSongLayout::nextLeft,UiSongLayout::navTop,
                  UiSongLayout::nextRight,UiSongLayout::navBottom},
               "WEITER >",false,UiSongLayout::canNext(songPage_,count));
   for(std::size_t offset=0;offset<UiSongLayout::perPage;++offset) {
     const auto index=first+offset;
     if(index>count)continue;
     const double sx=UiSongLayout::cellX(offset%UiSongLayout::columns);
     const double sy=UiSongLayout::cellY(offset/UiSongLayout::columns);
     char cell[40]{};
     if(index<count){
       const auto entry=controller_.songEntry(index);
       std::snprintf(cell,sizeof(cell),"%02u   %c   %s",
           static_cast<unsigned>(index+1),static_cast<char>('A'+entry.pattern),
           entry.steps==8?"1/2 TAKT":entry.steps==16?"1 TAKT":"2 TAKTE");
     }else std::snprintf(cell,sizeof(cell),"%02u   + EINTRAG",
             static_cast<unsigned>(index+1));
     drawControl(c,{sx,sy,sx+UiSongLayout::cellWidth,sy+UiSongLayout::cellHeight},
                 cell,index==songSlot_&&index<count);
   }
   c->setFont(VSTGUI::kNormalFont,12);c->setFontColor(accent);
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

 // Contextual menus are drawn AFTER the row grid; they cannot be obscured.
 if(fillMenuRow_>=0&&static_cast<std::size_t>(fillMenuRow_)<controller_.visibleRows()) {
   const double top=std::clamp(rowTopOnPage(static_cast<std::size_t>(fillMenuRow_)-firstRow_)+36.,
                               108.,fullHeight-230.);
   const char* items[]={"FILL 2","FILL 4","FILL 8","SNARE 2+4","FILL ALL","CLEAR","OFFBEAT HAT","GALOPP","SNARE + GHOST"};
   for(int i=0;i<9;++i)drawControl(c,{405,top+i*25.,614,top+i*25.+24.},items[i]);
 }
 if(popup_==1) {
   for(std::size_t i=0;i<UiCompactLayout::userZooms.size();++i) {
     char z[20]{};
     std::snprintf(z,sizeof(z),"%u%%",static_cast<unsigned>(
                   std::lround(UiCompactLayout::userZooms[i]*100.)));
     drawControl(c,{1098,39.+i*30.,1229,68.+i*30.},z,
                std::abs(editor_.getZoomFactor()-UiCompactLayout::userZooms[i])<0.001);
   }
 }else if(popup_==2) {
   const char* options[]={"KOPIEREN","EINFUEGEN","LOESCHEN","RUECKGAENGIG","VELO VARIIEREN","VELO 100"};
   for(int i=0;i<6;++i)
     drawControl(c,{782,86.+i*26.,968,111.+i*26.},options[i],
                 false,i!=3||controller_.canUndoCurrentPattern());
 }else if(popup_==3&&popupRow_<controller_.visibleRows()) {
   const double top=std::clamp(rowTopOnPage(popupRow_-firstRow_)+36.,108.,fullHeight-210.);
   for(unsigned i=0;i<16;++i) {
     char z[16]{};
     if(i==0)std::snprintf(z,sizeof(z),"MAIN");
     else std::snprintf(z,sizeof(z),"OUT %u",i+1);
     const auto col=i/8,row=i%8;
     drawControl(c,{413+col*102.,top+row*25.,511+col*102.,top+row*25.+24.},z,
                 controller_.rowOutput(popupRow_)==i);
   }
 }else if(popup_==5&&popupRow_<controller_.visibleRows()) {
   const double top=std::clamp(rowTopOnPage(popupRow_-firstRow_)+36.,108.,
                               std::max(108.,fullHeight-100.));
   const char* options[]={"SAMPLE LADEN","INSTRUMENT UMBENENNEN","SAMPLE ENTFERNEN"};
   for(int i=0;i<3;++i)
      drawControl(c,{78,top+i*30.,304,top+i*30.+29.},options[i]);
 }else if(popup_==4&&popupRow_<controller_.visibleRows()) {
   const double top=std::clamp(rowTopOnPage(popupRow_-firstRow_)+36.,108.,fullHeight-145.);
   for(unsigned i=0;i<5;++i) {
     char z[12]{};
     std::snprintf(z,sizeof(z),"CHOKE %u",i);
     drawControl(c,{515,top+i*27.,615,top+i*27.+26.},z,
                 controller_.rowChoke(popupRow_)==i);
   }
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
 const auto row=firstRow_+UiCompactLayout::rowAt(position.y,firstRow_,displayedRows(),velocityExpanded_);
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
 const double x=pos.x,y=pos.y;
 if(!(buttons&VSTGUI::kLButton))return VSTGUI::kMouseEventNotHandled;
 const auto done=VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 // Popups are modal: clicking outside dismisses them without changing a step.
 if(popup_!=0){
   const int kind=popup_;
   const auto target=popupRow_;
   popup_=0;popupRow_=kRows;
   bool acted=false,ok=true;
   if(kind==1&&x>=1098&&x<1229&&y>=39&&y<159) {
      const auto option=static_cast<std::size_t>((y-39)/30.);
      if(option<UiCompactLayout::userZooms.size()) {
        const auto factor=UiCompactLayout::userZooms[option];
        if(static_cast<DrumatorEditor&>(editor_).setZoomChecked(factor))
          controller_.rememberZoom(factor);
        else {status_="HOST HAT ZOOM ABGELEHNT";ok=false;}
        acted=true;
      }
   }else if(kind==2&&x>=782&&x<968&&y>=86&&y<242) {
      const int idx=static_cast<int>((y-86)/26.);
      if(idx>=0&&idx<6&&y<86.+idx*26.+25.){
        ok=idx==0?controller_.copyCurrentPattern():
            idx==1?controller_.pasteCurrentPattern():
            idx==2?controller_.clearCurrentPattern():
            idx==3?controller_.undoCurrentPattern():
            idx==4?controller_.varyPatternVelocity():
                   controller_.resetPatternVelocity();
        acted=true;
      }
   }else if(kind==3&&target<controller_.visibleRows()){
      const auto top=std::clamp(rowTopOnPage(target-firstRow_)+36.,108.,
                                getViewSize().getHeight()-210.);
      if(x>=413&&x<613&&y>=top&&y<top+200.){
        const int col=x>=515?1:0;
        const int cell=static_cast<int>((y-top)/25.);
        if(cell>=0&&cell<8&&x>=413+col*102&&x<511+col*102&&
           y<top+cell*25.+24.) {
          const auto selected=static_cast<unsigned>(col*8+cell);
          ok=controller_.setRowOutput(target,selected);acted=true;
        }
      }
   }else if(kind==4&&target<controller_.visibleRows()){
      const auto top=std::clamp(rowTopOnPage(target-firstRow_)+36.,108.,
                                getViewSize().getHeight()-145.);
      if(x>=515&&x<615&&y>=top&&y<top+135.){
        const int cell=static_cast<int>((y-top)/27.);
        if(cell>=0&&cell<5&&y<top+cell*27.+26.) {
          ok=controller_.setRowChoke(target,static_cast<unsigned>(cell));acted=true;
        }
      }
   }else if(kind==5&&target<controller_.visibleRows()){
     const auto top=std::clamp(rowTopOnPage(target-firstRow_)+36.,108.,
                              std::max(108.,getViewSize().getHeight()-100.));
     if(x>=78&&x<304&&y>=top&&y<top+90.){
       const auto option=static_cast<int>((y-top)/30.);
       if(option>=0&&option<3&&y<top+option*30.+29.){
         if(option==0)load(target);
         else if(option==1)editName(target);
         else if(!controller_.clearSample(target))ok=false;
         acted=true;
       }
     }
   }
   if(acted&&!ok)status_="AUSWAHL VOM PROZESSOR / HOST ABGELEHNT";
   invalid();return done;
 }
 if(fillMenuRow_>=0) {
   const auto target=static_cast<std::size_t>(fillMenuRow_);
   fillMenuRow_=-1;
   if(target>=firstRow_&&target<firstRow_+displayedRows()){
     const auto top=std::clamp(rowTopOnPage(target-firstRow_)+36.,108.,
                               getViewSize().getHeight()-230.);
     if(x>=405&&x<614&&y>=top&&y<top+225.){
       const auto selected=static_cast<std::size_t>((y-top)/25.);
       constexpr RowFill fills[]={RowFill::Every2,RowFill::Every4,RowFill::Every8,
           RowFill::Snare24,RowFill::All,RowFill::Clear,RowFill::Offbeat8,
           RowFill::Gallop,RowFill::SnareGhost};
       if(selected<9&&!controller_.fillRow(target,fills[selected]))
          status_="FILL NICHT UEBERNOMMEN";
     }
   }
   invalid();return done;
 }
 // Explicit pattern-length choice, independent of the operation popup.
 if(y>=8&&y<34&&((x>=460&&x<526)||(x>=533&&x<608))){
   const bool wantOne=x<526;
   const bool isOne=controller_.patternLength(controller_.currentPattern())==16;
   if(wantOne!=isOne&&!controller_.togglePatternLength(controller_.currentPattern()))
     status_="PATTERN-LAENGE NICHT UEBERNOMMEN";
   invalid();return done;
 }
 // Global controls: Rotary, direct pattern buttons, Song and two menus.
 if(x>=1120&&x<1250&&y>=7&&y<35){popup_=1;invalid();return done;}
 if(x>=782&&x<901&&y>=52&&y<82){popup_=2;invalid();return done;}
 if(x>=910&&x<991&&y>=52&&y<82) {
   if(!controller_.toggleSongMode())status_="SONG-MODUS NICHT UEBERNOMMEN";
   invalid();return done;
 }
 if(x>=998&&x<1098&&y>=52&&y<82) {
   songEditorOpen_=!songEditorOpen_;
   if(songEditorOpen_)adjustEditorHeight();
   invalid();return done;
 }
 if(y>=kPatternY&&y<kPatternY+30&&x>=kPatternX&&x<kPatternX+8*kPatternWidth){
   const auto idx=static_cast<std::size_t>((x-kPatternX)/kPatternWidth);
   if(x>=kPatternX+idx*kPatternWidth+49.)return VSTGUI::kMouseEventNotHandled;
   const bool ok=(buttons&VSTGUI::kShift)?
      controller_.togglePatternLength(idx):controller_.changePattern(idx);
   if(!ok)status_="PATTERN NICHT UEBERNOMMEN";
   invalid();return done;
 }
 for(int i=4;i<=6;++i){
   const auto center=UiCompactLayout::globalKnobCenters[static_cast<std::size_t>(i-4)];
   if(x>=center-18.&&x<center+18.&&y>=59&&y<94) {
      const int previous=i==4?static_cast<int>(controller_.swingPercent()):
                         i==5?static_cast<int>(controller_.humanizePercent()):
                              static_cast<int>(controller_.masterVolume());
      if((buttons&VSTGUI::kControl)||(buttons&VSTGUI::kDoubleClick)) {
        if(previous!=UiCompactLayout::defaultValue(i)&&
           !changeKnobValue(i,0,UiCompactLayout::defaultValue(i)))
           status_="REGLER RESET NICHT UEBERNOMMEN";
        invalid();return done;
      }
      activeKnob_=i;knobRow_=kRows;knobStartY_=y;knobStartValue_=previous;
       knobFine_=(buttons&VSTGUI::kShift)!=0;
      knobGeneration_=controller_.editGeneration();
      invalid();return VSTGUI::kMouseEventHandled;
   }
 }
 if(songEditorOpen_&&y>=101) {
   bool ok=true;
   const auto count=controller_.songCount();
   if(UiSongLayout::previousHit(x,y)){
     if(songPage_>0)--songPage_;
   }else if(UiSongLayout::nextHit(x,y)){
     if(UiSongLayout::canNext(songPage_,count))++songPage_;
   }else if(const auto slot=UiSongLayout::slotAt(x,y,songPage_);
             slot<SongChain::kCapacity) {
     if(slot<count){
       songSlot_=slot;
     }else if(slot==count){
       ok=controller_.adjustSongCount(1);
       if(ok){
         songSlot_=slot;
         songPage_=UiSongLayout::pageOf(slot);
       }
     }
   }else if(y>=277&&y<303) {
     const auto slot=std::min(songSlot_,count-1);
     const auto current=controller_.songEntry(slot);
     if(x>=115&&x<563) {
       const auto index=static_cast<std::size_t>((x-115)/56.);
       if(index<8&&x<115+index*56.+50.)
         ok=controller_.setSongEntry(slot,static_cast<std::uint8_t>(index),current.steps);
     }else if(x>=678&&x<996) {
       const auto index=static_cast<std::size_t>((x-678)/106.);
       if(index<3&&x<678+index*106.+100.) {
         const std::uint8_t lengths[]={8,16,32};
         ok=controller_.setSongEntry(slot,current.pattern,lengths[index]);
       }
     }else if(x>=1012&&x<1110) {
       if(count<SongChain::kCapacity){
         ok=controller_.adjustSongCount(1);
         if(ok){
           songSlot_=controller_.songCount()-1;
           songPage_=UiSongLayout::pageOf(songSlot_);
         }
       }
     }else if(x>=1118&&x<1239) {
       if(count>1){
         ok=controller_.adjustSongCount(-1);
         if(ok){
           if(songSlot_>=controller_.songCount())songSlot_=controller_.songCount()-1;
           songPage_=UiSongLayout::pageOf(songSlot_);
         }
       }
     }
   }
   if(!ok)status_="SONG-EINTRAG NICHT UEBERNOMMEN";
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }

 const double bottom=gridBottom();
 if(y>=bottom+5&&y<bottom+34){
   if(UiCompactLayout::hit(x,UiCompactLayout::rowAdd)){
     if(controller_.visibleRows()>=kRows)status_="MAXIMAL 16 INSTRUMENTE";
     else {
       const auto count=controller_.visibleRows();
       const auto result=tryRowResizeTransaction(
          [&](){return controller_.addRow();},
          [&](){return adjustEditorHeight();},
          [&](){return controller_.restoreVisibleRows(count);});
       if(result!=RowResizeResult::Applied)status_="ZEILE KONNTE NICHT HINZUGEFUEGT WERDEN";
     }
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::rowRemove)){
     const auto count=controller_.visibleRows();
     const auto result=tryRowResizeTransaction(
        [&](){return controller_.hideLastEmptyRow();},
        [&](){return adjustEditorHeight();},
        [&](){return controller_.restoreVisibleRows(count);});
     if(result!=RowResizeResult::Applied)
       status_="NUR LEERE LETZTE ZEILE ENTFERNBAR";
     invalid();return done;
   }
   if(controller_.visibleRows()>UiCompactLayout::perPage&&x>=1068&&x<1229){
     const auto previous=firstRow_;
     if(x<1145)firstRow_=firstRow_>=UiCompactLayout::perPage?
        firstRow_-UiCompactLayout::perPage:0;
     else if(x>=1152&&firstRow_+UiCompactLayout::perPage<controller_.visibleRows())
        firstRow_+=UiCompactLayout::perPage;
     if(firstRow_!=previous) {
       if(velocityRow_<firstRow_||velocityRow_>=firstRow_+displayedRows())
         velocityRow_=firstRow_;
       closeTransientMenus();
       if(!adjustEditorHeight()){firstRow_=previous;(void)adjustEditorHeight();}
     }
     invalid();return done;
   }
 }
 if(y>=kGridY&&y<bottom&&!songEditorOpen_){
   const auto idx=UiCompactLayout::rowAt(y,firstRow_,displayedRows(),velocityExpanded_);
   if(idx>=displayedRows())return VSTGUI::kMouseEventNotHandled;
   const auto row=firstRow_+idx;
   const auto top=rowTopOnPage(idx);
   if(velocityExpanded_[row]&&UiCompactLayout::velocityHit(y,top)) {
      const auto step=UiCompactLayout::stepAt(x);
      if(step>=0) {
        if(static_cast<std::size_t>(step)>=controller_.patternLength(controller_.currentPattern())) {
          status_="STEPS 17-32: 2 TAKTE WAEHLEN";
          invalid();return done;
        }
        velocityRow_=row;velocityStep_=static_cast<std::size_t>(step);
        laneDragPattern_=controller_.currentPattern();
        laneDragGeneration_=controller_.editGeneration();
        controller_.beginVelocityStroke(laneDragPattern_);
        laneDragActive_=true;laneErase_=(buttons&VSTGUI::kControl)!=0;
        if(!controller_.setStepVelocity(laneDragPattern_,row,velocityStep_,
               UiCompactLayout::velocityAt(y,top,laneErase_)))
          status_="VELOCITY NICHT UEBERNOMMEN";
        invalid();return VSTGUI::kMouseEventHandled;
      }
      return done;
   }
   if(!UiCompactLayout::mainHit(y,top))return done;
   if(x>=16&&x<74) {
     velocityRow_=row;status_="INSTRUMENT AUSGEWAEHLT";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::velocityToggle)&&
      y>=top+45&&y<top+79){
     velocityRow_=row;
     (void)toggleRowVelocity(row);invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::sampleMenu)&&
      y>=top+5&&y<top+35){
     popup_=5;popupRow_=row;invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::sample)&&y>=top+5&&y<top+35){
     if(buttons&VSTGUI::kShift)editName(row);
     else if(buttons&VSTGUI::kAlt){
       if(!controller_.clearSample(row))status_="SAMPLE NICHT ENTFERNT";
     }else load(row);
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::previous)&&y>=top+5&&y<top+35){
     if(controller_.stepSample(row,-1)!=Controller::LoadStatus::Accepted)
       status_="KEIN VORHERIGES SAMPLE";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::next)&&y>=top+5&&y<top+35){
     if(controller_.stepSample(row,1)!=Controller::LoadStatus::Accepted)
       status_="KEIN NAECHSTES SAMPLE";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::preview)&&y>=top+5&&y<top+35){
     if(!controller_.previewSample(row))status_="SAMPLE NICHT VORHOERBAR";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::fill)&&y>=top+5&&y<top+35){
     fillMenuRow_=static_cast<int>(row);invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::mute)&&y>=top+5&&y<top+35){
     if(!controller_.toggleRowGate(row,false))status_="MUTE NICHT UEBERNOMMEN";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::solo)&&y>=top+5&&y<top+35){
     if(!controller_.toggleRowGate(row,true))status_="SOLO NICHT UEBERNOMMEN";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::choke)&&y>=top+54&&y<top+86){
     popup_=4;popupRow_=row;invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::output)&&y>=top+5&&y<top+35){
     popup_=3;popupRow_=row;invalid();return done;
   }
   if(y>=top+45&&y<top+79){
     const auto step=UiCompactLayout::stepAt(x);
     if(step>=0){
       if(static_cast<std::size_t>(step)>=controller_.patternLength(controller_.currentPattern())) {
         status_="STEPS 17-32: 2 TAKTE WAEHLEN";
         invalid();return done;
       }
       velocityStep_=static_cast<std::size_t>(step);
       const bool ok=(buttons&VSTGUI::kShift)?
         controller_.cycleStepVelocity(controller_.currentPattern(),row,velocityStep_):
         controller_.toggleStep(controller_.currentPattern(),row,velocityStep_);
       if(!ok)status_="STEP NICHT UEBERNOMMEN";
       invalid();return done;
     }
   }
   for(int i=0;i<4;++i){
     const auto center=UiCompactLayout::rowKnobCenters[static_cast<std::size_t>(i)];
     if(x>=center-18.&&x<center+18.&&y>=top+51&&y<top+85){
       const int previous=i==0?static_cast<int>(controller_.rowGain(row)):
          i==1?controller_.rowPan(row):
          i==2?controller_.rowPitch(row):
               static_cast<int>(controller_.rowRandomVelocity(row));
       if((buttons&VSTGUI::kControl)||(buttons&VSTGUI::kDoubleClick)){
         const auto def=UiCompactLayout::defaultValue(i);
         if(previous!=def&&!changeKnobValue(i,row,def))
           status_="KLANGREGLER RESET NICHT UEBERNOMMEN";
         invalid();return done;
       }
       activeKnob_=i;knobRow_=row;knobStartY_=y;knobStartValue_=previous;
        knobFine_=(buttons&VSTGUI::kShift)!=0;
       knobGeneration_=controller_.editGeneration();
       invalid();return VSTGUI::kMouseEventHandled;
     }
   }
 }
 return VSTGUI::kMouseEventNotHandled;
}
VSTGUI::CMouseEventResult DrumatorView::onMouseMoved(
    VSTGUI::CPoint& pos,const VSTGUI::CButtonState& buttons) {
 if(activeKnob_>=0){
   if(knobGeneration_!=controller_.editGeneration()||!(buttons&VSTGUI::kLButton)){
     activeKnob_=-1;knobRow_=kRows;invalid();
     return VSTGUI::kMouseEventHandled;
   }
   const auto requested=UiCompactLayout::knobValueForSlot(
        activeKnob_,knobStartY_,pos.y,knobStartValue_,knobFine_);
   if(!changeKnobValue(activeKnob_,knobRow_,requested))
      status_="REGLERWERT NICHT UEBERNOMMEN";
   invalid();return VSTGUI::kMouseEventHandled;
 }
 if(!laneDragActive_)return VSTGUI::kMouseEventNotHandled;
 if(laneDragGeneration_!=controller_.editGeneration()||
    laneDragPattern_!=controller_.currentPattern()){
   if(laneDragGeneration_==controller_.editGeneration())
     controller_.endVelocityStroke(false);
   laneDragActive_=false;laneDragGeneration_=0;
   return VSTGUI::kMouseEventHandled;
 }
 if(!(buttons&VSTGUI::kLButton)){
   laneDragActive_=false;laneDragGeneration_=0;
   controller_.endVelocityStroke(true);
   return VSTGUI::kMouseEventHandled;
 }
 const auto step=UiCompactLayout::stepAt(pos.x);
 if(step<0||static_cast<std::size_t>(step)>=controller_.patternLength(laneDragPattern_)||
    songEditorOpen_||velocityRow_<firstRow_||
    velocityRow_>=firstRow_+displayedRows()||!velocityExpanded_[velocityRow_])
   return VSTGUI::kMouseEventHandled;
 const auto idx=velocityRow_-firstRow_;
 velocityStep_=static_cast<std::size_t>(step);
 const auto value=UiCompactLayout::velocityAt(pos.y,rowTopOnPage(idx),laneErase_);
 if(!controller_.setStepVelocity(laneDragPattern_,velocityRow_,velocityStep_,value))
   status_="VELOCITY-ZIEHEN NICHT UEBERNOMMEN";
 invalid();return VSTGUI::kMouseEventHandled;
}
VSTGUI::CMouseEventResult DrumatorView::onMouseUp(
    VSTGUI::CPoint&,const VSTGUI::CButtonState&) {
 if(activeKnob_>=0){
   activeKnob_=-1;knobRow_=kRows;
   invalid();return VSTGUI::kMouseEventHandled;
 }
 if(!laneDragActive_)return VSTGUI::kMouseEventNotHandled;
 const bool generationValid=laneDragGeneration_==controller_.editGeneration();
 const bool fresh=generationValid&&laneDragPattern_==controller_.currentPattern();
 laneDragActive_=false;laneDragGeneration_=0;
 if(fresh)controller_.endVelocityStroke(true);
 else if(generationValid)controller_.endVelocityStroke(false);
 return VSTGUI::kMouseEventHandled;
}
VSTGUI::CMouseEventResult DrumatorView::onMouseCancel(){
 activeKnob_=-1;knobRow_=kRows;
 detailDragControl_=-1;detailDragRow_=kRows;
 const bool fresh=laneDragActive_&&
                  laneDragGeneration_==controller_.editGeneration();
 laneDragActive_=false;laneDragGeneration_=0;
 if(fresh)controller_.endVelocityStroke(false);
 invalid();return VSTGUI::kMouseEventHandled;
}

}
