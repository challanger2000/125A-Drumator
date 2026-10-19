#include "DrumatorView.h"
#include "DrumatorEditor.h"
#include "core/RowResizeTransaction.h"
#include "core/CompactRowViewport.h"
#include "DrumatorController.h"
#include "core/StepVelocity.h"
#include "core/RowMixControl.h"
#include "core/Utf8Path.h"
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
#include <string_view>
#include <filesystem>
#include <system_error>

namespace drumator {
namespace {
constexpr double kLabelX=16.0,kGridX=622.0,kGridY=108.0;
constexpr double kCellWidth=19.0,kRowHeight=29.0;
constexpr double kGridRight=kGridX+32*kCellWidth;
constexpr double kMinHeight=326.0;
double contentHeight(std::size_t rows) {
 return std::max(kMinHeight,kGridY+rows*kRowHeight+99.0);
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
    if(local.y<kGridY||local.y>=kGridY+view_.visibleRowsForHit()*kRowHeight||
       local.x<kLabelX||local.x>kGridRight)return false;
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
 const bool hasPath=DrumatorDropTarget::extractSamplePath(data.drag,path);
 status_=hasPath?"D&D: DATEIPFAD ERKANNT - AUF INSTRUMENTENZEILE ZIEHEN":
                 "D&D: EREIGNIS EMPFANGEN, ABER KEIN WAV/FLAC-DATEIPFAD";
 invalid();
 return hasPath?VSTGUI::DragOperation::Copy:VSTGUI::DragOperation::None;
}
VSTGUI::DragOperation DrumatorView::onDragMove(VSTGUI::DragEventData data) {
 std::string path;
 return DrumatorDropTarget::extractSamplePath(data.drag,path)
   ?VSTGUI::DragOperation::Copy:VSTGUI::DragOperation::None;
}
void DrumatorView::onDragLeave(VSTGUI::DragEventData) {}
bool DrumatorView::onDrop(VSTGUI::DragEventData data) {
 return acceptFileDrop(data);
}
std::size_t DrumatorView::visibleRowsForHit() const noexcept {
 return displayedRows();
}
std::size_t DrumatorView::displayedRows() const noexcept {
 return compactViewport_?CompactRowViewport::count(controller_.visibleRows(),firstRow_):controller_.visibleRows();
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
   compactViewport_=true;
   firstRow_=CompactRowViewport::first(controller_.visibleRows(),firstRow_);
   fillMenuRow_=-1;
   status_="HOST-RESIZE ABGELEHNT: ZEILEN MIT < > WECHSELN";
   invalid();return false;
 }
 compactViewport_=false;firstRow_=0;
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
 c->drawRect({0,0,1280,contentHeight(controller_.visibleRows())},VSTGUI::kDrawFilled);
 c->setFont(VSTGUI::kNormalFont,18);
 c->setFontColor(letters);
 c->drawString("125A  /  DRUMATOR",VSTGUI::CRect(16,8,1000,32),
               VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,12);
 c->setFontColor(accent);
 c->drawString("SAMPLE LADEN: NAME KLICKEN / DATEI ZIEHEN  |  FILL: PRO ZEILE  |  SHIFT+STEP: VELOCITY",VSTGUI::CRect(16,47,990,67),
               VSTGUI::kLeftText);
 const double footerY=kGridY+displayedRows()*kRowHeight+39.0;
 c->drawString(status_.c_str(),VSTGUI::CRect(16,footerY,1240,footerY+21),
               VSTGUI::kLeftText);
 c->setFontColor(letters);
 for(std::size_t pattern=0;pattern<8;++pattern) {
   const double x=kPatternX+pattern*kPatternWidth;
   char label[2]={static_cast<char>('A'+pattern),0};
   drawControl(c,{x,kHeaderY,x+54,kHeaderY+30},label,
               pattern==controller_.currentPattern());
 }
 if(compactViewport_){
   c->setFontColor(accent);
   char page[64]{};
   std::snprintf(page,sizeof(page),"ZEILEN %u-%u / %u",static_cast<unsigned>(firstRow_+1),
                 static_cast<unsigned>(firstRow_+displayedRows()),static_cast<unsigned>(controller_.visibleRows()));
   c->drawString(page,VSTGUI::CRect(820,39,1030,69));
   c->drawString("<",VSTGUI::CRect(1005,39,1030,69));
   c->drawString(">",VSTGUI::CRect(1033,39,1060,69));
 }
 const bool zoom150=editor_.getZoomFactor()>1.25;
 drawControl(c,{1070,39,1127,69},"100%",!zoom150);
 drawControl(c,{1136,39,1217,69},"150%",zoom150);
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
 std::snprintf(songSummary,sizeof(songSummary),"CHAIN: %u EINTRAEGE  |  1/2 / 1 / 2 TAKTE",
               static_cast<unsigned>(controller_.songCount()));
 c->setFontColor(accent);
 c->drawString(songSummary,VSTGUI::CRect(866,74,1030,98),VSTGUI::kLeftText);
 c->setFont(VSTGUI::kNormalFont,12);
 for(std::size_t row=firstRow_;row<firstRow_+displayedRows();++row) {
   const double y=kGridY+(row-firstRow_)*kRowHeight;
   c->setFillColor(row%2==0?panel:background);
   c->drawRect({10,y,1246,y+kRowHeight-1},VSTGUI::kDrawFilled);
   c->setFontColor(letters);
   // Visible per-row FILL button, not a hidden action behind a row number.
   drawControl(c,{16,y+2,48,y+26},"FILL");
   const auto& name=controller_.label(row);
   // Fit the filename in the fixed row area; preserve the extension where possible.
   std::string visibleName=name;
   // Reserve space for previous/next sample buttons and the output selector.
   if(visibleName.size()>16) {
     // Truncate on UTF-8 codepoint boundaries, never inside a character.
     auto prefixEnd=std::min<std::size_t>(9,name.size());
     while(prefixEnd>0&&prefixEnd<name.size()&&
           (static_cast<unsigned char>(name[prefixEnd])&0xC0u)==0x80u)
       --prefixEnd;
     auto suffixBegin=name.size()-4;
     while(suffixBegin<name.size()&&
           (static_cast<unsigned char>(name[suffixBegin])&0xC0u)==0x80u)
       ++suffixBegin;
     visibleName=name.substr(0,prefixEnd)+"..."+name.substr(suffixBegin);
   }
   c->drawString(visibleName.c_str(),VSTGUI::CRect(kLabelX+36,y,202,y+26),
                 VSTGUI::kLeftText);
   drawControl(c,{203,y+2,216,y+26},"<");
   drawControl(c,{216,y+2,229,y+26},">");
   // sample name rendering above has its own bounds

   char busLabel[16]{};
   if(controller_.rowOutput(row)==0)std::snprintf(busLabel,sizeof(busLabel),"MAIN");
   else std::snprintf(busLabel,sizeof(busLabel),"O%02u",controller_.rowOutput(row)+1);
   drawControl(c,{230,y+2,275,y+26},busLabel);
   char value[32]{};
   c->setFontColor(accent);
   std::snprintf(value,sizeof(value),"VOL %u",controller_.rowGain(row));
   c->drawString(value,VSTGUI::CRect(280,y,345,y+26),VSTGUI::kLeftText);
   std::snprintf(value,sizeof(value),"PAN %+d",controller_.rowPan(row));
   c->drawString(value,VSTGUI::CRect(350,y,415,y+26),VSTGUI::kLeftText);
   std::snprintf(value,sizeof(value),"P %+d",controller_.rowPitch(row));
   c->drawString(value,VSTGUI::CRect(420,y,475,y+26),VSTGUI::kLeftText);
   // Visible decrement/increment zones: left/right half of each control.
   c->setFontColor(letters);
   for(double left:{280.0,350.0,420.0}) {
      const double width=left==420.0?55.0:65.0;
      c->drawString("-",VSTGUI::CRect(left,y+15,left+width/2.0,y+28));
      c->drawString("+",VSTGUI::CRect(left+width/2.0,y+15,left+width,y+28));
   }
   drawControl(c,{478,y+3,500,y+24},"M",controller_.isRowMuted(row));
   drawControl(c,{507,y+3,529,y+24},"S",controller_.isRowSolo(row));
   char chokeText[8]{};
   std::snprintf(chokeText,sizeof(chokeText),"%u",controller_.rowChoke(row));
   drawControl(c,{535,y+3,558,y+24},chokeText,controller_.rowChoke(row)!=0);
   char randomLabel[24]{};
   std::snprintf(randomLabel,sizeof(randomLabel),"RND %u%%",
       controller_.rowRandomVelocity(row));
   drawControl(c,{560,y+3,619,y+25},randomLabel,
               controller_.rowRandomVelocity(row)>0);
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
     if(step%4==0) {
       c->setFrameColor(border);
       c->drawRect({x+1,y+3,x+18,y+23},VSTGUI::kDrawStroked);
     }
   }
 }
 const double buttonsY=kGridY+displayedRows()*kRowHeight+5.0;
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
 const char* actions[]={"COPY","PASTE","CLEAR","UNDO"};
 for(int i=0;i<4;++i) {
   const double x=480+i*90.;
   drawControl(c,{x,buttonsY,x+85,buttonsY+29},actions[i],
               i==3&&controller_.canUndoCurrentPattern());
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
 if(fillMenuRow_>=0&&static_cast<std::size_t>(fillMenuRow_)<controller_.visibleRows()) {
    const double top=std::max(0.0,std::min(kGridY+(fillMenuRow_-static_cast<int>(firstRow_))*kRowHeight,getViewSize().getHeight()-230.0));
    const char* names[]={"FILL 2","FILL 4","FILL 8","SNARE 2+4","FILL ALL","CLEAR","OFFBEAT HIHAT","GALOPP","SNARE + GHOST"};
    for(int i=0;i<9;++i){const double y=top+i*25.;drawControl(c,{650,y,819,y+24},names[i]);}
 }
 // Full 64-slot chain board. Pattern and length choices are direct buttons.
 // Board overlays the step matrix, leaving drag-and-drop behavior untouched
 // in normal PATTERN view.
 if(songEditorOpen_) {
   c->setFillColor(background);
   c->drawRect({8,101,1252,contentHeight(controller_.visibleRows())-1},VSTGUI::kDrawFilled);
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
 if(position.x<kLabelX||position.x>kGridRight||
    position.y<kGridY||position.y>=kGridY+displayedRows()*kRowHeight) {
   status_="D&D FEHLER: AUSSERHALB DER INSTRUMENTENZEILEN";
   invalid();return false;
 }
 const auto row=firstRow_+static_cast<std::size_t>((position.y-kGridY)/kRowHeight);
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
   const double top=std::max(0.0,std::min(kGridY+(fillMenuRow_-static_cast<int>(firstRow_))*kRowHeight,getViewSize().getHeight()-230.0));
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
   if(x<1032)firstRow_=CompactRowViewport::previous(controller_.visibleRows(),firstRow_);
   else firstRow_=CompactRowViewport::next(controller_.visibleRows(),firstRow_);
   fillMenuRow_=-1;invalid();
   return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(y>=74&&y<98&&x>=1140&&x<1228){
   const int amount=(buttons&VSTGUI::kControl)?
     -static_cast<int>(controller_.masterVolume()):(x<1182?-5:5);
   if(!controller_.adjustMasterVolume(amount))status_="MASTER LEVEL NICHT UEBERNOMMEN";
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }
 if(y>=74&&y<98&&x>=540&&x<850) {
   bool ok=true;
   if(x<638)ok=controller_.toggleSongMode();
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
       if(index<8)ok=controller_.setSongEntry(slot,static_cast<std::uint8_t>(index),current.steps);
     }else if(x>=678&&x<996) {
       const auto index=static_cast<std::size_t>((x-678)/106.);
       if(index<3) {
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
     controller_.changePattern(index);invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
 }
 if(y>=39&&y<69) {
   double requested=0.;
   if(x>=1070&&x<1127)requested=1.0;
   else if(x>=1136&&x<1217)requested=1.5;
   if(requested>0.) {
     if(static_cast<DrumatorEditor&>(editor_).setZoomChecked(requested))
       controller_.rememberZoom(requested);
     else status_="HOST HAT ZOOM ABGELEHNT";
     invalid();
     return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
 }
 const double actionsY=kGridY+displayedRows()*kRowHeight+5.0;
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
                            (x>=1024?controller_.resetPatternVelocity():true);
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
 if(y>=kGridY&&y<kGridY+displayedRows()*kRowHeight) {
   const auto row=firstRow_+static_cast<std::size_t>((y-kGridY)/kRowHeight);
   if(x>=16&&x<48) {
     fillMenuRow_=static_cast<int>(row);
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if((x>=280&&x<345)||(x>=350&&x<415)||(x>=420&&x<475)) {
      const bool pitch=x>=420;
      const bool pan=x>=350&&x<415;
      // Direct left/right half controls, modifier-free at the default zoom.
      const double start=pitch?420.0:(pan?350.0:280.0);
      const double width=pitch?55.0:65.0;
      const bool reset=(buttons&VSTGUI::kControl)!=0;
      const auto control=pitch?RowControl::Pitch:(pan?RowControl::Pan:RowControl::Volume);
      const int current=pitch?controller_.rowPitch(row):
                        pan?controller_.rowPan(row):static_cast<int>(controller_.rowGain(row));
      const int next=rowControlNext(control,current,x<start+width/2.0?-1:1,reset);
      const int delta=next-current;
      if(delta!=0) {
         const bool ok=pitch?controller_.adjustRowPitch(row,delta):
                             controller_.adjustRowMix(row,pan,delta);
         if(!ok)status_="REGLERWERT NICHT UEBERNOMMEN";
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
      if(!controller_.cycleRowChoke(row,(buttons&VSTGUI::kShift)?-1:1))status_="CHOKE NICHT UEBERNOMMEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=203&&x<230&&(buttons&VSTGUI::kAlt)==0) {
     const auto result=controller_.stepSample(row,x<216?-1:1);
     status_=result==Controller::LoadStatus::Accepted?"SAMPLE GEWECHSELT":"SAMPLEWECHSEL NICHT MOEGLICH";
     invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=230&&x<275&&(buttons&VSTGUI::kAlt)==0) {
      if(!controller_.cycleRowOutput(row))status_="AUSGANG NICHT UEBERNOMMEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=kLabelX&&x<203&&(buttons&VSTGUI::kAlt)!=0) {
      if(controller_.clearSample(row))status_="SAMPLE ENTFERNT - STEPS BLEIBEN";
      else status_="SAMPLE KONNTE NICHT ENTFERNT WERDEN";
      invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=kLabelX&&x<203) {
      load(row);return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   if(x>=560&&x<619) {
      const auto current=static_cast<int>(controller_.rowRandomVelocity(row));
      const int delta=(buttons&VSTGUI::kControl)?-current:(x<589?-10:10);
      if(!controller_.adjustRandomVelocity(row,delta))
         status_="RANDOM VELOCITY NICHT UEBERNOMMEN";
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
}
