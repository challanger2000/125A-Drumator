#include "DrumatorView.h"
#include "BrandLogoGeometry.h"
#include "core/UiWaveEditorLayout.h"
#include "vstgui/lib/cgraphicspath.h"
#include <cstdlib>
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
#include "vstgui/lib/cgraphicstransform.h"
#include "vstgui/lib/cbitmap.h"
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
#include <cmath>
#include <string>
#include <string_view>
#include <filesystem>
#include <exception>
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
// Blue instrument modules are drawn natively so they follow row expansion and zoom.
// Desaturated blue steel and graphite. Amber remains a step-only status.
const VSTGUI::CColor rowBlueA{19,40,58,255};
const VSTGUI::CColor rowBlueB{17,36,54,255};
const VSTGUI::CColor rowBlueSelected{24,51,71,255};
const VSTGUI::CColor rowBlueBorder{53,91,121,255};
const VSTGUI::CColor stepWellBlue{6,17,30,255};
const VSTGUI::CColor velocityWellBlue{9,28,43,255};

// Exact 125A vector logo from the authoritative brand master, rendered
// natively through VSTGUI (SVG paths contain only M, L, Z commands).
// The source master is 1774x887; keep its proportions unchanged.
VSTGUI::CRect editorRect(const UiWaveEditorLayout::Rect& r){
 return {r.left,r.top,r.right,r.bottom};
}
void drawBrandLogo(VSTGUI::CDrawContext* c) {
 constexpr double scale=32./887.;
 constexpr double x0=16.,y0=2.;
 for(const auto& shape: kBrandLogoPaths){
   auto* path=c->createGraphicsPath();
   if(!path)continue;
   const char* cursor=shape.path;
   while(*cursor){
     while(*cursor==' '||*cursor=='\\n'||*cursor=='\\t')++cursor;
     if(!*cursor)break;
     const char operation=*cursor++;
     if(operation=='Z'){path->closeSubpath();continue;}
     if(operation!='M'&&operation!='L')break;
     while(*cursor==' ')++cursor;
     char* endX=nullptr;
     const double x=std::strtod(cursor,&endX);
     if(endX==cursor)break;
     cursor=endX;
     while(*cursor==' ')++cursor;
     char* endY=nullptr;
     const double y=std::strtod(cursor,&endY);
     if(endY==cursor)break;
     cursor=endY;
     if(operation=='M')path->beginSubpath(x0+x*scale,y0+y*scale);
     else path->addLine(x0+x*scale,y0+y*scale);
   }
   c->setFillColor(shape.red?VSTGUI::CColor{215,25,32,255}:
                             VSTGUI::CColor{217,217,217,255});
   c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathFilledEvenOdd);
   path->forget();
 }
}

// Centralized VSTGUI button renderer: consistent visible frame, fill,
// contrast and selected/disabled states, with fixed hit regions.
// Hardware recessed field shared by header, rows and sample displays.
// Visual-only drawing: does not change any interactive hit region.
void drawInset(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,
               const VSTGUI::CColor& interior) {
 c->setFillColor(VSTGUI::CColor{4,13,24,255});
 c->drawRect({r.left-1.,r.top-1.,r.right+1.,r.bottom+1.},VSTGUI::kDrawFilled);
 c->setFillColor(interior);c->drawRect(r,VSTGUI::kDrawFilled);
 c->setFillColor(VSTGUI::CColor{2,9,18,255});
 c->drawRect({r.left,r.top,r.right,r.top+2.},VSTGUI::kDrawFilled);
 c->drawRect({r.left,r.top,r.left+2.,r.bottom},VSTGUI::kDrawFilled);
 c->setFillColor(VSTGUI::CColor{49,84,110,255});
 c->drawRect({r.left+1.,r.bottom-1.,r.right,r.bottom},VSTGUI::kDrawFilled);
 c->drawRect({r.right-1.,r.top+2.,r.right,r.bottom},VSTGUI::kDrawFilled);
}
// Floating hardware menu chassis. Only paints a bezel and a short contact
// shadow behind the existing controls; their positions/hit targets are intact.
void drawPopupPlate(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r) {
 c->setFillColor(VSTGUI::CColor{1,8,15,255});
 c->drawRect({r.left+4.,r.top+4.,r.right+4.,r.bottom+4.},VSTGUI::kDrawFilled);
 c->setFillColor(VSTGUI::CColor{10,27,40,255});
 c->drawRect(r,VSTGUI::kDrawFilled);
 c->setFrameColor(VSTGUI::CColor{90,132,156,255});
 c->drawRect(r,VSTGUI::kDrawStroked);
 c->setFillColor(VSTGUI::CColor{121,160,180,255});
 c->drawRect({r.left+2.,r.top+2.,r.right-2.,r.top+3.},VSTGUI::kDrawFilled);
 c->setFillColor(VSTGUI::CColor{3,13,23,255});
 c->drawRect({r.left+2.,r.bottom-4.,r.right-2.,r.bottom-2.},VSTGUI::kDrawFilled);
}
void drawControl(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,
                 const char* title,bool selected=false,bool enabled=true) {
 // Contact shadow and raised keycap, inset by a further pixel when selected.
 c->setFillColor(VSTGUI::CColor{2,13,23,255});
 c->drawRect(r,VSTGUI::kDrawFilled);
 if(r.getWidth()>6.&&r.getHeight()>8.) {
   const double press=selected?2.:1.;
   const VSTGUI::CRect face{r.left+2.,r.top+press,r.right-2.,
                            r.bottom-(selected?2.:4.)};
   c->setFillColor(VSTGUI::CColor{1,9,18,255});
   c->drawRect({r.left+2.,r.bottom-4.,r.right-1.,r.bottom-1.},VSTGUI::kDrawFilled);
   // Deep skirt remains visible beneath the cap even on selected buttons.
   c->setFillColor(selected?VSTGUI::CColor{20,63,93,255}:
                             VSTGUI::CColor{15,35,51,255});
   c->drawRect({r.left+3.,r.bottom-4.,r.right-3.,r.bottom-2.},VSTGUI::kDrawFilled);
   // Raised perimeter edge: a light-catching top lip and a deep lower skirt.
   c->setFillColor(VSTGUI::CColor{118,155,179,255});
   c->drawRect({r.left+3.,r.top+1.,r.right-3.,r.top+2.},VSTGUI::kDrawFilled);
   const double split=face.top+face.getHeight()*.53;
   c->setFillColor(!enabled?VSTGUI::CColor{37,46,54,255}:
           selected?VSTGUI::CColor{29,79,111,255}:VSTGUI::CColor{64,96,119,255});
   c->drawRect({face.left,face.top,face.right,split},VSTGUI::kDrawFilled);
   c->setFillColor(!enabled?VSTGUI::CColor{23,33,44,255}:
           selected?VSTGUI::CColor{22,65,96,255}:VSTGUI::CColor{25,45,64,255});
   c->drawRect({face.left,split,face.right,face.bottom},VSTGUI::kDrawFilled);
   c->setFillColor(selected?VSTGUI::CColor{13,41,62,255}:
                             VSTGUI::CColor{136,167,187,255});
   c->drawRect({face.left+1.,face.top,face.right-1.,face.top+1.},VSTGUI::kDrawFilled);
   // Machined left chamfer and black shadowed right sidewall.
   c->setFillColor(selected?VSTGUI::CColor{46,104,141,255}:
                             VSTGUI::CColor{103,139,163,255});
   c->drawRect({face.left+1.,face.top+2.,face.left+2.,face.bottom-2.},
               VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{7,21,34,255});
   c->drawRect({face.right-2.,face.top+3.,face.right,face.bottom-1.},
               VSTGUI::kDrawFilled);
   c->setFillColor(selected?VSTGUI::CColor{73,152,195,255}:
                             VSTGUI::CColor{13,27,39,255});
   c->drawRect({face.left+1.,face.bottom-2.,face.right-1.,face.bottom},VSTGUI::kDrawFilled);
   c->setFrameColor(!enabled?VSTGUI::CColor{46,58,66,255}:
        selected?VSTGUI::CColor{87,170,211,255}:VSTGUI::CColor{70,105,133,255});
   c->drawRect(face,VSTGUI::kDrawStroked);
 }
 c->setFrameColor(VSTGUI::CColor{5,23,38,255});
 c->drawRect(r,VSTGUI::kDrawStroked);
 // Cold-blue pilot lights show actual selected/engaged control state.
 // Their small upper-right sockets avoid the centered legends on M/S keys.
 if(selected&&enabled&&r.getWidth()>=30.&&r.getHeight()>=24.) {
   const double px=r.right-8.,py=r.top+5.;
   c->setFillColor(VSTGUI::CColor{2,18,31,255});
   c->drawRect({px-1.,py-1.,px+5.,py+5.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{40,112,158,255});
   c->drawRect({px,py,px+4.,py+4.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{136,224,251,255});
   c->drawRect({px+1.,py+1.,px+3.,py+3.},VSTGUI::kDrawFilled);
 }
 c->setFont(VSTGUI::kNormalFont,12);
 c->setFontColor(!enabled?VSTGUI::CColor{143,156,169,255}:
                 selected?VSTGUI::CColor{245,252,255,255}:letters);
 c->drawString(title,{r.left,r.top+(selected?1.:0.),r.right,r.bottom+(selected?1.:0.)});
}


void drawHardwareStep(VSTGUI::CDrawContext* c,double sx,double y,
                      bool lit,bool disabled,bool downbeat) {
 // True recessed socket and shallow travel inside the unchanged 19px pitch.
 const VSTGUI::CRect socket{sx+1.,y+69.,sx+18.,y+103.};
 c->setFillColor(VSTGUI::CColor{2,9,17,255});
 c->drawRect(socket,VSTGUI::kDrawFilled);
 c->setFrameColor(downbeat?VSTGUI::CColor{68,102,123,255}:
                             VSTGUI::CColor{31,58,77,255});
 c->drawRect(socket,VSTGUI::kDrawStroked);
 c->setFillColor(VSTGUI::CColor{1,11,20,255});
 c->drawRect({sx+3.,y+73.,sx+17.,y+102.},VSTGUI::kDrawFilled);
 // The outside of each key's raised cap casts an asymmetric socket shadow.
 c->setFillColor(VSTGUI::CColor{0,5,11,255});
 c->drawRect({sx+16.,y+76.,sx+18.,y+103.},VSTGUI::kDrawFilled);
 const double t=y+(lit?72.:71.),b=y+(lit?98.:97.);
 c->setFillColor(disabled?VSTGUI::CColor{32,40,49,255}:
                lit?VSTGUI::CColor{173,70,22,255}:
                    VSTGUI::CColor{49,68,83,255});
 c->drawRect({sx+3.,t,sx+16.,b},VSTGUI::kDrawFilled);
 c->setFillColor(disabled?VSTGUI::CColor{46,54,64,255}:
                lit?VSTGUI::CColor{241,126,35,255}:
                    VSTGUI::CColor{73,95,112,255});
 c->drawRect({sx+4.,t+2.,sx+14.,b-2.},VSTGUI::kDrawFilled);
 c->setFillColor(disabled?VSTGUI::CColor{66,74,82,255}:
                lit?VSTGUI::CColor{255,185,85,255}:
                    VSTGUI::CColor{123,146,162,255});
 c->drawRect({sx+4.,t+1.,sx+15.,t+3.},VSTGUI::kDrawFilled);
 // Fine top bevel and left metallic return suggest a separate keycap
 // rather than a flat illuminated rectangle.
 c->setFillColor(disabled?VSTGUI::CColor{62,71,80,255}:
                lit?VSTGUI::CColor{255,208,112,255}:
                    VSTGUI::CColor{165,187,201,255});
 c->drawRect({sx+5.,t,sx+14.,t+1.},VSTGUI::kDrawFilled);
 c->drawRect({sx+3.,t+3.,sx+4.,b-3.},VSTGUI::kDrawFilled);
 // Black sidewall makes the key read as a physical raised cap.
 c->setFillColor(disabled?VSTGUI::CColor{21,28,35,255}:
                lit?VSTGUI::CColor{107,44,16,255}:
                    VSTGUI::CColor{24,43,54,255});
 c->drawRect({sx+14.,t+4.,sx+16.,b},VSTGUI::kDrawFilled);
 c->setFillColor(disabled?VSTGUI::CColor{15,25,32,255}:
                lit?VSTGUI::CColor{121,47,15,255}:
                    VSTGUI::CColor{13,28,38,255});
 c->drawRect({sx+4.,b-2.,sx+14.,b},VSTGUI::kDrawFilled);
 // Every physical step has a recessed indicator lens, including OFF
 // and disabled keys. Previously the aperture appeared only for ON keys.
 // This is paint only, inside the existing 19px step pitch.
 c->setFillColor(VSTGUI::CColor{2,9,17,255});
 c->drawRect({sx+5.,y+64.,sx+15.,y+69.},VSTGUI::kDrawFilled);
 c->setFillColor(disabled?VSTGUI::CColor{13,23,32,255}:
                           VSTGUI::CColor{25,45,58,255});
 c->drawRect({sx+6.,y+65.,sx+14.,y+68.},VSTGUI::kDrawFilled);
 c->setFillColor(disabled?VSTGUI::CColor{31,43,51,255}:
                           VSTGUI::CColor{58,80,93,255});
 c->drawRect({sx+7.,y+65.,sx+13.,y+66.},VSTGUI::kDrawFilled);
 if(lit){
  // Dedicated amber pilot lamp in the key bank's upper recess.
  // It reflects the actual step state; there are no decorative fake LEDs.
  c->setFillColor(VSTGUI::CColor{78,31,11,255});
  c->drawRect({sx+5.,y+65.,sx+14.,y+69.},VSTGUI::kDrawFilled);
  c->setFillColor(VSTGUI::CColor{255,140,37,255});
  c->drawRect({sx+6.,y+66.,sx+13.,y+68.},VSTGUI::kDrawFilled);
  c->setFillColor(VSTGUI::CColor{255,229,147,255});
  c->drawRect({sx+8.,y+66.,sx+11.,y+67.},VSTGUI::kDrawFilled);
  c->setFillColor(VSTGUI::CColor{255,219,147,255});
  c->drawRect({sx+7.,y+93.,sx+12.,y+97.},VSTGUI::kDrawFilled);
  c->setFillColor(VSTGUI::CColor{255,248,222,255});
  c->drawRect({sx+8.,y+94.,sx+11.,y+96.},VSTGUI::kDrawFilled);
 }
}

void drawPanelScrew(VSTGUI::CDrawContext* c,VSTGUI::CBitmap* bitmap,double x,double y,double angle) {
 if(!bitmap||bitmap->getWidth()!=32.||bitmap->getHeight()!=32.)return;
 // Win32 bitmap renderer cannot reliably draw a rotated PNG. Use a
 // scale/translate-only transform (as for the tested knob filmstrip).
 (void)angle;
 constexpr double factor=18./32.;
 VSTGUI::CDrawContext::Transform transform(*c,
   VSTGUI::CGraphicsTransform().scale(factor,factor)
     .translate(x-9.,y-9.));
 bitmap->draw(c,{0.,0.,32.,32.});
}
void drawRotary(VSTGUI::CDrawContext* c,VSTGUI::CBitmap* sprite,double cx,double cy,double fraction,
                const char* title,const char* value,bool selected=false,
                bool rowModule=false) {
 fraction=std::clamp(fraction,0.,1.);
 c->setFont(VSTGUI::kNormalFont,11);c->setFontColor(accent);
 // Global controls share their historic layout; a bezel here would collide
 // with their title/nameplate. Row labels are deliberately placed higher.
 const double titleTop=rowModule?UiCompactLayout::rowKnobLabelTop:-37.;
 const double titleBottom=rowModule?UiCompactLayout::rowKnobLabelBottom:-24.;
 c->drawString(title,{cx-46.,cy+titleTop,cx+46.,cy+titleBottom});
 if(rowModule) {
   const double r=UiCompactLayout::rowKnobMountRadius;
   c->setFillColor(VSTGUI::CColor{4,15,26,255});
   c->drawEllipse({cx-r,cy-r,cx+r,cy+r},VSTGUI::kDrawFilled);
   c->setFrameColor(VSTGUI::CColor{90,136,165,255});
   c->drawEllipse({cx-r+1.,cy-r+1.,cx+r-1.,cy+r-1.},VSTGUI::kDrawStroked);
   c->setFrameColor(VSTGUI::CColor{11,29,43,255});
   c->drawEllipse({cx-r+3.,cy-r+3.,cx+r-3.,cy+r-3.},VSTGUI::kDrawStroked);
   // Restrained functional blue scale ticks on the free left half of the
   // metal bezel. Right side stays clear for the existing numeric value.
   // Their brightness follows the real normalized knob value, not decoration.
   constexpr double kPi=3.14159265358979323846;
   const int activeTicks=static_cast<int>(std::lround(fraction*9.));
   for(int tick=0;tick<9;++tick) {
     const double angle=(-142.+13.*tick)*kPi/180.;
     const double dx=std::sin(angle),dy=-std::cos(angle);
     c->setFrameColor(tick<activeTicks?
         VSTGUI::CColor{53,164,218,255}:VSTGUI::CColor{27,58,77,255});
     c->setLineWidth(tick<activeTicks?2.:1.);
     c->drawLine({cx+dx*31.,cy+dy*31.},
                 {cx+dx*34.,cy+dy*34.});
   }
   c->setLineWidth(1.);
 }
 // A bitmap object can exist even when its PNG resource was not loaded,
 // notably in native offscreen QA outside the installed VST3 bundle.
 // Treat that as unavailable and paint an actual usable fallback control.
 if(auto* frames=dynamic_cast<VSTGUI::CMultiFrameBitmap*>(sprite);
    frames&&frames->getWidth()==125.&&frames->getHeight()==12625.) {
   const auto frame=frames->normalizedValueToFrameIndex(static_cast<float>(fraction));
   // JKnobMan source is 125px wide but only the inner 81px is opaque.
   // A 75px full frame displays a real 48.6px diameter knob, matching
   // the 50px clickable hit zone without covering adjacent labels.
   constexpr double frameRadius=37.5;
   const double scale=2.*frameRadius/125.;
   VSTGUI::CDrawContext::Transform transform(*c,
     VSTGUI::CGraphicsTransform().scale(scale,scale)
       .translate(cx-frameRadius,cy-frameRadius));
   frames->drawFrame(c,frame,{0.,0.});
   if(selected) {
     c->setFrameColor(accent);c->setLineWidth(1.);
     c->drawEllipse({22.,22.,103.,103.},VSTGUI::kDrawStroked);
   }
 }else{
   // Visible fallback if a host did not load the sprite resource.
   c->setFillColor(panel);
   c->drawEllipse({cx-25.,cy-25.,cx+25.,cy+25.},VSTGUI::kDrawFilled);
   c->setFrameColor(letters);
   c->drawEllipse({cx-25.,cy-25.,cx+25.,cy+25.},VSTGUI::kDrawStroked);
   const auto angle=(-135.+270.*fraction)*3.14159265358979323846/180.;
   c->setFrameColor(on);c->setLineWidth(2.5);
   c->drawLine({cx,cy},{cx+std::sin(angle)*20.,cy-std::cos(angle)*20.});
 }
 c->setLineWidth(1.);c->setFont(VSTGUI::kNormalFont,12);c->setFontColor(letters);
 c->drawString(value,{cx+29.,cy-12.,cx+72.,cy+13.},VSTGUI::kLeftText);
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
    try {
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
    }catch(const std::exception&) {return false;}
}
 DrumatorView& view_;
};
VSTGUI::DragOperation DrumatorView::onDragEnter(VSTGUI::DragEventData data) {
 std::string path;
 const bool allowed=sampleDropTargetAt(data.pos.x,data.pos.y);
 const bool hasPath=allowed&&DrumatorDropTarget::extractSamplePath(data.drag,path);
 switch(classifyDrop(allowed,hasPath)) {
   case DropFeedback::TargetBlocked:
     status_="D&D: INVALID OR OBSCURED SAMPLE TARGET";break;
   case DropFeedback::InvalidFilePath:
     status_="D&D: INVALID WAV/FLAC FILE PATH";break;
   case DropFeedback::ValidTarget:
     status_="D&D: DROP ON INSTRUMENT ROW";break;
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
bool DrumatorView::moveScroll(double y,double grab) {
 const auto total=controller_.visibleRows(),shown=displayedRows();
 if(shown==0||total<=shown)return false;
 const double top=UiCompactLayout::scrollTop;
 const double length=std::max(1.,gridBottom()-8.-top);
 const double thumb=std::clamp(length*static_cast<double>(shown)/
                      static_cast<double>(total),28.,length);
 const double travel=std::max(1.,length-thumb);
 const auto next=static_cast<std::size_t>(std::lround(
     std::clamp((y-grab-top)/travel,0.,1.)*static_cast<double>(total-shown)));
 if(next==firstRow_)return true;
 const auto previous=firstRow_;firstRow_=next;
 if(!adjustEditorHeight()){firstRow_=previous;(void)adjustEditorHeight();return false;}
 if(velocityRow_<firstRow_||velocityRow_>=firstRow_+displayedRows())velocityRow_=firstRow_;
 closeTransientMenus();invalid();return true;
}
bool DrumatorView::toggleRowVelocity(std::size_t row) {
 if(row>=controller_.visibleRows())return false;
 if(laneDragActive_){controller_.endVelocityStroke(false,this);laneDragActive_=false;}
 const auto previous=velocityExpanded_[row];
 velocityExpanded_[row]=!previous;
 if(!adjustEditorHeight()){
   velocityExpanded_[row]=previous;
   (void)adjustEditorHeight();
   status_="HOST REJECTED VELOCITY LANE RESIZE";
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
 if(sampleEditorOpen_||songEditorOpen_||popup_!=0||fillMenuRow_>=0)return false;
 const auto index=UiCompactLayout::rowAt(y,firstRow_,displayedRows(),velocityExpanded_);
 if(index>=displayedRows())return false;
 const auto top=rowTopOnPage(index);
 return UiCompactLayout::mainHit(y,top)&&x>=UiCompactLayout::sample.left&&
        x<UiCompactLayout::sample.right;
}
DrumatorView::~DrumatorView() {
 if(backgroundBitmap_){backgroundBitmap_->forget();backgroundBitmap_=nullptr;}
 if(knobBitmap_){knobBitmap_->forget();knobBitmap_=nullptr;}
 if(screwBitmap_){screwBitmap_->forget();screwBitmap_=nullptr;}
 if(sampleEditorOpen_)controller_.stopSampleEditPreview();
 controller_.unregisterView(this);
}
bool DrumatorView::removed(VSTGUI::CView* parent) {
 draggingScroll_=false;
 if(sampleEditorOpen_){controller_.stopSampleEditPreview();sampleEditorOpen_=false;
   sampleEditorRow_=kRows;sampleEditorDrag_=0;}
 if(laneDragActive_){controller_.endVelocityStroke(false,this);laneDragActive_=false;}
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
 if(!controller_.renameRow(editingNameRow_,text))status_="ROW NAME NOT APPLIED";
 else status_="ROW NAME SAVED";
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
 if(sampleEditorOpen_)controller_.stopSampleEditPreview();
 sampleEditorOpen_=false;sampleEditorRow_=kRows;sampleEditorDrag_=0;
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
 if(UiSongLayout::first(songPage_)>controller_.songCount())
   songPage_=UiSongLayout::pageOf(songSlot_);
 if(relayout&&getParentView())adjustEditorHeight();
 else {
   // A row-count edit from another editor must fit THIS editor's current
   // client area. Recompute accessible paging without another host resize:
   // the initiating editor retains its existing resize/rollback transaction.
   const auto total=controller_.visibleRows();
   const auto page=std::min(UiCompactLayout::perPage,total-firstRow_);
   std::size_t fit=0;
   for(std::size_t count=1;count<=page;++count) {
     if(UiCompactLayout::editorHeight(firstRow_,count,velocityExpanded_)>
        getViewSize().getHeight())break;
     fit=count;
   }
   compactPageRows_=std::max(std::size_t{1},fit);
   compactViewport_=total>UiCompactLayout::perPage||compactPageRows_<page;
   invalid();
 }
}
bool DrumatorView::attached(VSTGUI::CView* parent) {
 const auto result=CView::attached(parent);
 if(result)adjustEditorHeight();
 return result;
}
bool DrumatorView::adjustEditorHeight() {
 const auto total=controller_.visibleRows();
 if(firstRow_>=total)firstRow_=total>0?total-1:0;
 const auto page=std::min(UiCompactLayout::perPage,total-firstRow_);
 // The Song board needs only its own 370px surface.
 const auto height=songEditorOpen_?370.:
     std::max(370.,UiCompactLayout::editorHeight(firstRow_,page,velocityExpanded_));
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
   status_="HOST WINDOW TOO SMALL: PAGE ROWS WITH ARROWS";
   invalid();return false;
 }
 compactPageRows_=page;
 compactViewport_=total>page;
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
 backgroundBitmap_=new VSTGUI::CBitmap(VSTGUI::CResourceDescription("abstraktes_blaues_metall_mit_sanftem_glanz.png"));
 knobBitmap_=new VSTGUI::CMultiFrameBitmap(VSTGUI::CResourceDescription("125A_Drumator_Knob.png"),{{125.,125.},101,1});
 screwBitmap_=new VSTGUI::CBitmap(VSTGUI::CResourceDescription("125A_Drumator_Screw.png"));
}
void DrumatorView::draw(VSTGUI::CDrawContext* c) {
 const auto bottom=gridBottom();
 const auto fullHeight=getViewSize().getHeight();
 // The former bright-blue bitmap was still visible behind the fascia
 // despite subsequent low-contrast line overlays. The approved hardware
 // reference uses dark desaturated blue steel, not saturated cyan stripes.
 // Paint only the exposed chassis with deterministic 1px micro-grain;
 // leave all tested controls, trays and hit rectangles untouched.
 c->setFillColor(VSTGUI::CColor{6,25,44,255});
 c->drawRect({0,0,1280,fullHeight},VSTGUI::kDrawFilled);
 const auto brushSteel=[&](int first,int last,bool topBand) {
   for(int yy=first;yy<last;++yy) {
     const int grain=(yy*11+3)%17;
     c->setFillColor(topBand?
       (grain==0?VSTGUI::CColor{9,34,59,255}:
        grain==5?VSTGUI::CColor{5,28,51,255}:
                 VSTGUI::CColor{7,31,55,255}):
       (grain==0?VSTGUI::CColor{9,32,54,255}:
        grain==5?VSTGUI::CColor{5,25,45,255}:
                 VSTGUI::CColor{7,29,50,255}));
     c->drawRect({3.,static_cast<double>(yy),1277.,
                  static_cast<double>(yy+1)},VSTGUI::kDrawFilled);
   }
 };
 brushSteel(2,std::min(40,static_cast<int>(fullHeight)),true);
 if(fullHeight>bottom)
   brushSteel(std::max(0,static_cast<int>(bottom)),
              static_cast<int>(fullHeight),false);
 // Dark control deck with precise machined nameplate seam.
 c->setFillColor(VSTGUI::CColor{11,27,42,255});
 c->drawRect({0,42,1280,108},VSTGUI::kDrawFilled);
 // Layered steel fascia edge: upper specular bevel and dark undercut.
 c->setFillColor(VSTGUI::CColor{95,151,187,255});
 c->drawRect({5.,1.,1275.,2.},VSTGUI::kDrawFilled);
 c->setFillColor(VSTGUI::CColor{6,18,31,255});
 c->drawRect({4.,37.,1276.,40.},VSTGUI::kDrawFilled);
 c->setFillColor(VSTGUI::CColor{4,15,27,255});
 c->drawRect({0,40,1280,43},VSTGUI::kDrawFilled);
 c->setFillColor(VSTGUI::CColor{76,126,162,255});
 c->drawRect({4,40,1276,41},VSTGUI::kDrawFilled);
 drawInset(c,{17,46,228,103},VSTGUI::CColor{12,31,46,255});
 drawInset(c,{323,46,774,99},VSTGUI::CColor{10,27,43,255});
 drawInset(c,{779,46,1106,99},VSTGUI::CColor{10,27,43,255});
 drawInset(c,{1124,46,1253,103},VSTGUI::CColor{12,31,46,255});
 c->setFont(VSTGUI::kNormalFont,20);c->setFontColor(letters);
 c->drawString("125A DRUMATOR",{465,5,815,35},VSTGUI::kCenterText);
 drawBrandLogo(c);
 c->setFont(VSTGUI::kNormalFont,11);c->setFontColor(accent);
 c->drawString("PATTERN",{180,13,276,33},VSTGUI::kLeftText);
 const bool oneBar=controller_.patternLength(controller_.currentPattern())==16;
 drawControl(c,{UiCompactLayout::barOne.left,8,UiCompactLayout::barOne.right,34},"1 BAR",oneBar);
 drawControl(c,{UiCompactLayout::barTwo.left,8,UiCompactLayout::barTwo.right,34},"2 BARS",!oneBar);
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
 drawRotary(c,knobBitmap_,UiCompactLayout::globalKnobCenters[0],76,controller_.swingPercent()/50.,"SWING",text,activeKnob_==4);
 std::snprintf(text,sizeof(text),"%u%%",controller_.humanizePercent());
 drawRotary(c,knobBitmap_,UiCompactLayout::globalKnobCenters[1],76,controller_.humanizePercent()/100.,"HUMANIZE",text,activeKnob_==5);
 std::snprintf(text,sizeof(text),"%u%%",controller_.masterVolume());
 drawRotary(c,knobBitmap_,UiCompactLayout::globalKnobCenters[2],76,controller_.masterVolume()/200.,"MASTER",text,activeKnob_==6);
 // Four full-fledged rows per page: the selection has its own independent hit box.
 for(std::size_t i=0;i<displayedRows();++i) {
   const auto row=firstRow_+i;
   const double y=rowTopOnPage(i);
   const bool chosen=row==velocityRow_;
   const auto rowH=UiCompactLayout::baseRow+(velocityExpanded_[row]?UiCompactLayout::velocityRow:0.);
   // Individual graphite instrument trays with subtle selectable blue edge.
   c->setFillColor(VSTGUI::CColor{3,13,23,255});
   c->drawRect({8,y+2.,1261,y+rowH-2.},VSTGUI::kDrawFilled);
   c->setFillColor(chosen?rowBlueSelected:(i%2?rowBlueB:rowBlueA));
   c->drawRect({10,y+4.,1260,y+rowH-5.},VSTGUI::kDrawFilled);
   // Satin graphite face inside the blue steel carrier. A narrow exposed
   // perimeter and opposing specular/undercut edges give each row a distinct
   // hardware tray without altering its size or any clickable rectangle.
   // Satin faceplate: upper shoulder catches light; lower deck recedes.
   // Pure paint, with unchanged widget geometry and interaction rectangles.
   c->setFillColor(chosen?VSTGUI::CColor{26,49,63,255}:
                            VSTGUI::CColor{21,40,53,255});
   c->drawRect({12.,y+7.,1258.,y+40.},VSTGUI::kDrawFilled);
   c->setFillColor(chosen?VSTGUI::CColor{19,39,52,255}:
                            VSTGUI::CColor{14,29,41,255});
   c->drawRect({12.,y+40.,1258.,y+rowH-9.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{91,117,131,255});
   c->drawRect({11.,y+5.,1259.,y+6.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{46,72,89,255});
   c->drawRect({11.,y+6.,12.,y+rowH-9.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{5,17,27,255});
   c->drawRect({1258.,y+7.,1260.,y+rowH-7.},VSTGUI::kDrawFilled);
   c->drawRect({11.,y+rowH-9.,1260.,y+rowH-6.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{44,68,84,255});
   c->drawRect({12.,y+rowH-10.,1258.,y+rowH-9.},VSTGUI::kDrawFilled);
   c->setFrameColor(chosen?accent:rowBlueBorder);
   c->drawRect({9,y+3.,1261,y+rowH-4.},VSTGUI::kDrawStroked);
   c->setFillColor(chosen?VSTGUI::CColor{83,178,209,255}:
                            VSTGUI::CColor{37,88,124,255});
   c->drawRect({12,y+10.,15,y+rowH-11.},VSTGUI::kDrawFilled);
   // Machined undercut between the upper button shelf and rotary deck.
   c->setFillColor(VSTGUI::CColor{3,13,24,255});
   c->drawRect({18.,y+43.,615.,y+45.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{55,84,102,255});
   c->drawRect({18.,y+45.,615.,y+46.},VSTGUI::kDrawFilled);
   // Readable row functions versus visually separated step sequencer.
   drawInset(c,{18,y+46.,615,y+120.},VSTGUI::CColor{16,34,49,255});
   drawInset(c,{620,y+28.,1230,y+115.},VSTGUI::CColor{13,31,47,255});
   // Dark dividing seam and a single silver-blue return: two separate
   // recessed modules, not a painted line running through the controls.
   c->setFillColor(VSTGUI::CColor{2,10,18,255});
   c->drawRect({616.,y+30.,618.,y+115.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{69,99,116,255});
   c->drawRect({618.,y+31.,619.,y+113.},VSTGUI::kDrawFilled);
   drawInset(c,{kGridX-3.,y+64.,kGridRight+4.,y+110.},stepWellBlue);
   // Eight separate four-step key banks. Alternate the dark graphite
   // inserts and machine a slim upper lip; all geometry stays strictly
   // behind the existing key sockets and outside their 19px hit regions.
   for(int bank=0;bank<8;++bank) {
     const double bx=kGridX+bank*4*kCellWidth;
     const double right=bx+4*kCellWidth;
     c->setFillColor(bank%2?VSTGUI::CColor{9,23,36,255}:
                             VSTGUI::CColor{13,29,43,255});
     c->drawRect({bx+1.,y+68.,right-1.,y+106.},VSTGUI::kDrawFilled);
     c->setFillColor(bank%2?VSTGUI::CColor{39,67,85,255}:
                             VSTGUI::CColor{49,82,101,255});
     c->drawRect({bx+2.,y+68.,right-2.,y+69.},VSTGUI::kDrawFilled);
     c->setFillColor(VSTGUI::CColor{2,11,20,255});
     c->drawRect({right-1.,y+70.,right,y+106.},VSTGUI::kDrawFilled);
   }
   // The 16/17 division is a measure seam, not an interactive separator.
   const double barSeam=kGridX+16*kCellWidth;
   c->setFillColor(VSTGUI::CColor{89,129,151,255});
   c->drawRect({barSeam,y+64.,barSeam+1.,y+69.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{4,12,22,255});
   c->drawRect({barSeam,y+104.,barSeam+1.,y+109.},VSTGUI::kDrawFilled);
   // Eight physically indexed four-step groups. Markers live above/below
   // the key sockets and never consume a step's existing 19px hit target.
   for(int group=4;group<32;group+=4) {
     const double gx=kGridX+group*kCellWidth;
     c->setFillColor(VSTGUI::CColor{63,92,108,255});
     c->drawRect({gx,y+61.,gx+1.,y+68.},VSTGUI::kDrawFilled);
     c->setFillColor(VSTGUI::CColor{30,58,76,255});
     c->drawRect({gx,y+105.,gx+1.,y+109.},VSTGUI::kDrawFilled);
   }
   char name[16]{};
   std::snprintf(name,sizeof(name),"%02u",static_cast<unsigned>(row+1));
   drawControl(c,{16,y+9,74,y+40},name,chosen);
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
   // Dark recessed file-name display, no imaginary audio waveforms.
   drawInset(c,{UiCompactLayout::sample.left,y+9,UiCompactLayout::sample.right,y+40},
             VSTGUI::CColor{3,16,26,255});
   c->setFontColor(letters);
   c->drawString(fitted.c_str(),{UiCompactLayout::sample.left+5,y+9,UiCompactLayout::sample.right-2,y+40},VSTGUI::kLeftText);
   drawControl(c,{275,y+9,301,y+40},"...");
    drawControl(c,{UiCompactLayout::edit.left,y+9,UiCompactLayout::edit.right,y+40},"EDIT",false,controller_.hasSample(row));
   drawControl(c,{303,y+9,329,y+40},"<");
   drawControl(c,{332,y+9,358,y+40},">");
   drawControl(c,{362,y+9,398,y+40},"PLAY");
   drawControl(c,{403,y+9,452,y+40},"FILL");
   drawControl(c,{457,y+9,491,y+40},"M",controller_.isRowMuted(row));
   drawControl(c,{496,y+9,530,y+40},"S",controller_.isRowSolo(row));
   const auto out=controller_.rowOutput(row);
   if(out==0)std::snprintf(name,sizeof(name),"MAIN v");
   else std::snprintf(name,sizeof(name),"OUT %u v",out+1);
   drawControl(c,{535,y+9,615,y+40},name,popup_==3&&popupRow_==row);
   std::snprintf(name,sizeof(name),"CH%u v",controller_.rowChoke(row));
   c->setFont(VSTGUI::kNormalFont,10);c->setFontColor(accent);
   c->drawString("CHOKE",{570,y+52,615,y+73});
   drawControl(c,{570,y+73,615,y+105},name,popup_==4&&popupRow_==row);
   const double centers[]={133.,254.,375.,496.};
   const int values[]={static_cast<int>(controller_.rowGain(row)),controller_.rowPan(row),
                       controller_.rowPitch(row),static_cast<int>(controller_.rowRandomVelocity(row))};
   const char* labels[]={"VOL","PAN","PITCH"};
   for(int k=0;k<3;++k){
     const int lo=UiCompactLayout::minValue(k),hi=UiCompactLayout::maxValue(k);
     const int v=values[k];
     char desc[24]{};
     if(k==0)std::snprintf(desc,sizeof(desc),"%d%%",v);
     else if(k==1){
       if(v==0)std::snprintf(desc,sizeof(desc),"C");
       else std::snprintf(desc,sizeof(desc),"%c%d",v<0?'L':'R',std::abs(v));
     }else std::snprintf(desc,sizeof(desc),"%+d st",v);
     drawRotary(c,knobBitmap_,centers[k],y+88,
                static_cast<double>(v-lo)/static_cast<double>(hi-lo),
                labels[k],desc,chosen&&activeKnob_==k,true);
   }
   // Nonzero legacy realtime modulation remains visible and controllable.
   if(controller_.rowRandomVelocity(row)>0){
     char legacy[32]{};
     std::snprintf(legacy,sizeof(legacy),"LEGACY RT %u%%",
                   static_cast<unsigned>(controller_.rowRandomVelocity(row)));
     drawRotary(c,knobBitmap_,centers[3],y+88,controller_.rowRandomVelocity(row)/100.,
                "RANDOM RT",legacy,chosen&&activeKnob_==3,true);
   }else{
     drawControl(c,{448,y+73,544,y+105},"RANDOM VEL");
   }
   const auto steps=controller_.patternLength(controller_.currentPattern());
   for(std::size_t step=0;step<32;++step) {
     const auto sx=kGridX+step*kCellWidth;
     const auto velocity=controller_.velocity(controller_.currentPattern(),row,step);
     const bool inactive=step>=steps;
     if(step%4==0) {
       char index[8]{};
       std::snprintf(index,sizeof(index),"%u",static_cast<unsigned>(step+1));
       // Bare group numbers on the blue row, no unwanted dark boxes.
       c->setFont(VSTGUI::kNormalFont,12);
       c->setFontColor(inactive?VSTGUI::CColor{146,165,180,255}:
                                VSTGUI::CColor{226,242,253,255});
       c->drawString(index,{sx+3.,y+36.,sx+32.,y+57.},VSTGUI::kLeftText);
     }
     // Only actual step velocity lights the amber LED aperture.
     drawHardwareStep(c,sx,y,velocity>0&&!inactive,inactive,step%4==0);
   }
   c->setFont(VSTGUI::kNormalFont,10);c->setFontColor(accent);
   c->drawString("VEL",{1233,y+44,1259,y+62});
   drawControl(c,{1233,y+70,1259,y+105},velocityExpanded_[row]?"^":"v",velocityExpanded_[row]);
   if(velocityExpanded_[row]){
     c->setFillColor(velocityWellBlue);
     c->drawRect({16,y+130,1266,y+159},VSTGUI::kDrawFilled);
     c->setFont(VSTGUI::kNormalFont,11);c->setFontColor(accent);
     c->drawString("VELOCITY",{78,y+130,170,y+158},VSTGUI::kLeftText);
     if(row==velocityRow_) {
       char current[48]{};
       std::snprintf(current,sizeof(current),"STEP %u : %u / 127",
         static_cast<unsigned>(velocityStep_+1),
         static_cast<unsigned>(controller_.velocity(controller_.currentPattern(),row,velocityStep_)));
       c->drawString(current,{195,y+130,390,y+158},VSTGUI::kLeftText);
     }
     for(std::size_t step=0;step<32;++step){
       const double sx=kGridX+step*kCellWidth;
       const auto vel=controller_.velocity(controller_.currentPattern(),row,step);
       // Recessed velocity lane and luminous cyan bar; retain old input geometry.
       const bool disabled=step>=steps;
       c->setFillColor(VSTGUI::CColor{5,24,46,255});
       c->drawRect({sx+2.,y+133.,sx+17.,y+156.},VSTGUI::kDrawFilled);
       c->setFillColor(disabled?VSTGUI::CColor{29,45,61,255}:
                                  VSTGUI::CColor{34,75,106,255});
       c->drawRect({sx+3.,y+134.,sx+16.,y+155.},VSTGUI::kDrawFilled);
       if(vel>0&&!disabled) {
         const double top=y+155.-21.*static_cast<double>(vel)/127.;
         c->setFillColor(VSTGUI::CColor{29,151,196,255});
         c->drawRect({sx+3.,top,sx+16.,y+155.},VSTGUI::kDrawFilled);
         c->setFillColor(VSTGUI::CColor{109,228,244,255});
         c->drawRect({sx+5.,top,sx+8.,y+155.},VSTGUI::kDrawFilled);
         c->setFillColor(VSTGUI::CColor{163,246,250,255});
         c->drawRect({sx+3.,top,sx+16.,top+1.},VSTGUI::kDrawFilled);
       }
     }
   }
 }
 // Draw the native-instrument rail only when rows are hidden.
 if(!songEditorOpen_&&controller_.visibleRows()>displayedRows()){
   const double top=UiCompactLayout::scrollTop;
   const double length=std::max(1.,bottom-8.-top);
   const double thumb=std::clamp(length*static_cast<double>(displayedRows())/
                       static_cast<double>(controller_.visibleRows()),28.,length);
   const double thumbTop=top+(length-thumb)*static_cast<double>(firstRow_)/
                       static_cast<double>(controller_.visibleRows()-displayedRows());
   c->setFillColor(VSTGUI::CColor{5,23,42,255});
   c->drawRect({UiCompactLayout::scrollLeft,top,UiCompactLayout::scrollRight,top+length},VSTGUI::kDrawFilled);
   c->setFrameColor(VSTGUI::CColor{64,114,154,255});
   c->drawRect({UiCompactLayout::scrollLeft,top,UiCompactLayout::scrollRight,top+length},VSTGUI::kDrawStroked);
   c->setFillColor(VSTGUI::CColor{33,90,129,255});
   c->drawRect({UiCompactLayout::scrollLeft+2.,thumbTop+1.,UiCompactLayout::scrollRight-2.,thumbTop+thumb-1.},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{147,207,230,255});
   c->drawRect({UiCompactLayout::scrollLeft+3.,thumbTop+2.,UiCompactLayout::scrollLeft+5.,thumbTop+thumb-2.},VSTGUI::kDrawFilled);
 }
 // Fixed machined fascia and recessed status window.
 c->setFillColor(VSTGUI::CColor{10,28,42,255});
 c->drawRect({0,bottom,1280,bottom+UiCompactLayout::footer},VSTGUI::kDrawFilled);
 c->setFillColor(VSTGUI::CColor{77,120,147,255});
 c->drawRect({9,bottom+1.,1271,bottom+2.},VSTGUI::kDrawFilled);
 drawInset(c,{217,bottom+5.,1022,bottom+35.},VSTGUI::CColor{9,27,41,255});
 drawControl(c,{UiCompactLayout::rowAdd.left,bottom+5,UiCompactLayout::rowAdd.right,bottom+34},"ADD ROW");
 drawControl(c,{UiCompactLayout::rowRemove.left,bottom+5,UiCompactLayout::rowRemove.right,bottom+34},"REMOVE ROW",false,controller_.canHideLast());
 char range[64]{};
 std::snprintf(range,sizeof(range),"%u-%u / %u INSTRUMENTS",
   static_cast<unsigned>(firstRow_+1),static_cast<unsigned>(firstRow_+displayedRows()),
   static_cast<unsigned>(controller_.visibleRows()));
 c->setFontColor(accent);c->setFont(VSTGUI::kNormalFont,12);
 c->drawString(range,{222,bottom+5,410,bottom+34},VSTGUI::kLeftText);
 c->drawString(status_.c_str(),{414,bottom+5,1016,bottom+34},VSTGUI::kLeftText);
 if(controller_.visibleRows()>displayedRows()) {
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
   const bool selectionVisible=songSlot_>=first&&
       songSlot_<first+UiSongLayout::perPage&&songSlot_<count;
   char header[128]{};
   std::snprintf(header,sizeof(header),
       "SONG CHAIN   %02u-%02u / 64   |   SELECT SLOT, THEN PATTERN AND LENGTH",
       static_cast<unsigned>(first+1),static_cast<unsigned>(first+UiSongLayout::perPage));
   c->setFont(VSTGUI::kNormalFont,12);c->setFontColor(letters);
   c->drawString(header,{16,105,975,133},VSTGUI::kLeftText);
   drawControl(c,{UiSongLayout::previousLeft,UiSongLayout::navTop,
                  UiSongLayout::previousRight,UiSongLayout::navBottom},
               "< PREVIOUS",false,songPage_>0);
   drawControl(c,{UiSongLayout::nextLeft,UiSongLayout::navTop,
                  UiSongLayout::nextRight,UiSongLayout::navBottom},
               "NEXT >",false,UiSongLayout::canNext(songPage_,count));
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
           entry.steps==8?"1/2 BAR":entry.steps==16?"1 BAR":"2 BARS");
     }else std::snprintf(cell,sizeof(cell),"%02u   + ENTRY",
             static_cast<unsigned>(index+1));
     drawControl(c,{sx,sy,sx+UiSongLayout::cellWidth,sy+UiSongLayout::cellHeight},
                 cell,index==songSlot_&&index<count);
   }
   c->setFont(VSTGUI::kNormalFont,11);c->setFontColor(accent);
   char selectionLabel[80]{};
   if(selectionVisible)
     std::snprintf(selectionLabel,sizeof(selectionLabel),"POSITION %02u SELECTED",static_cast<unsigned>(songSlot_+1));
   else std::snprintf(selectionLabel,sizeof(selectionLabel),"ADD OR SELECT AN ENTRY FIRST");
   c->drawString(selectionLabel,{16,246,490,272},VSTGUI::kLeftText);
   c->setFont(VSTGUI::kNormalFont,12);c->setFontColor(accent);
   c->drawString("PATTERN:",VSTGUI::CRect(16,277,120,301),VSTGUI::kLeftText);
   for(std::size_t idx=0;idx<8;++idx) {
     const auto chosen=selectionVisible&&controller_.songEntry(songSlot_).pattern==idx;
     const double bx=115+idx*56.0;
     char letter[2]{static_cast<char>('A'+idx),0};
     drawControl(c,{bx,277,bx+50,303},letter,chosen,selectionVisible);
   }
   c->setFontColor(accent);
   c->drawString("LENGTH:",VSTGUI::CRect(594,277,681,303),VSTGUI::kLeftText);
   const char* lenNames[]={"1/2 BAR","1 BAR","2 BARS"};
   const std::uint8_t lenSteps[]={8,16,32};
   for(int idx=0;idx<3;++idx) {
     const auto chosen=selectionVisible&&controller_.songEntry(songSlot_).steps==lenSteps[idx];
     const double bx=678+idx*106.;
     drawControl(c,{bx,277,bx+100,303},lenNames[idx],chosen,selectionVisible);
   }
   drawControl(c,{1012,277,1110,303},"ADD ENTRY",false,count<64);
   drawControl(c,{1118,277,1239,303},"REMOVE LAST",false,count>1);
 }

 // Contextual menus are drawn AFTER the row grid; they cannot be obscured.
 if(fillMenuRow_>=0&&static_cast<std::size_t>(fillMenuRow_)<controller_.visibleRows()) {
   const double top=UiCompactLayout::popupTop(
       rowTopOnPage(static_cast<std::size_t>(fillMenuRow_)-firstRow_),fullHeight,230.);
   const char* items[]={"FILL 2","FILL 4","FILL 8","SNARE 2+4","FILL ALL","CLEAR","OFFBEAT HAT","GALLOP","SNARE + GHOST"};
   drawPopupPlate(c,{401.,top-4.,618.,top+229.});
   for(int i=0;i<9;++i)drawControl(c,{405,top+i*25.,614,top+i*25.+24.},items[i]);
 }
 if(popup_==1) {
   drawPopupPlate(c,{1094.,35.,1233.,42.+30.*UiCompactLayout::userZooms.size()});
   for(std::size_t i=0;i<UiCompactLayout::userZooms.size();++i) {
     char z[20]{};
     std::snprintf(z,sizeof(z),"%u%%",static_cast<unsigned>(
                   std::lround(UiCompactLayout::userZooms[i]*100.)));
     drawControl(c,{1098,39.+i*30.,1229,68.+i*30.},z,
                std::abs(editor_.getZoomFactor()-UiCompactLayout::userZooms[i])<0.001);
   }
 }else if(popup_==2) {
   drawPopupPlate(c,{778.,82.,972.,272.});
   const char* options[]={ "COPY","PASTE","CLEAR","UNDO","RANDOM VELOCITY","VELOCITY 100","DUPLICATE"};
   for(int i=0;i<7;++i)
     drawControl(c,{782,86.+i*26.,968,111.+i*26.},options[i],
                 false,(i!=3||controller_.canUndoCurrentPattern())&&
                       (i!=6||controller_.canDuplicateCurrentPattern()));
 }else if(popup_==3&&popupRow_<controller_.visibleRows()) {
   const double top=UiCompactLayout::popupTop(rowTopOnPage(popupRow_-firstRow_),fullHeight,210.);
   drawPopupPlate(c,{409.,top-4.,617.,top+204.});
   for(unsigned i=0;i<16;++i) {
     char z[16]{};
     if(i==0)std::snprintf(z,sizeof(z),"MAIN");
     else std::snprintf(z,sizeof(z),"OUT %u",i+1);
     const auto col=i/8,row=i%8;
     drawControl(c,{413+col*102.,top+row*25.,511+col*102.,top+row*25.+24.},z,
                 controller_.rowOutput(popupRow_)==i);
   }
 }else if(popup_==5&&popupRow_<controller_.visibleRows()) {
   const double top=UiCompactLayout::popupTop(rowTopOnPage(popupRow_-firstRow_),fullHeight,130.);
   drawPopupPlate(c,{74.,top-4.,308.,top+94.});
   const char* options[]={"LOAD SAMPLE","RENAME ROW","REMOVE SAMPLE"};
   for(int i=0;i<3;++i)
      drawControl(c,{78,top+i*30.,304,top+i*30.+29.},options[i]);
 }else if(popup_==4&&popupRow_<controller_.visibleRows()) {
   const double top=UiCompactLayout::popupTop(rowTopOnPage(popupRow_-firstRow_),fullHeight,145.);
   drawPopupPlate(c,{511.,top-4.,619.,top+139.});
   for(unsigned i=0;i<5;++i) {
     char z[12]{};
     std::snprintf(z,sizeof(z),"CHOKE %u",i);
     drawControl(c,{515,top+i*27.,615,top+i*27.+26.},z,
                 controller_.rowChoke(popupRow_)==i);
   }
 }

 if(sampleEditorOpen_) {
   // Modal overlay, fully inside the existing 1280-wide view at 100% zoom.
   // Match the Drumator blue-metal shell; keep editor geometry and hit zones.
   c->setFillColor(VSTGUI::CColor{10,27,42,255});
   c->drawRect({230,100,1050,605},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{24,52,72,255});
   c->drawRect({234,104,1046,151},VSTGUI::kDrawFilled);
   c->setFillColor(VSTGUI::CColor{95,147,177,255});
   c->drawRect({234,104,1046,106},VSTGUI::kDrawFilled);
   c->setFrameColor(VSTGUI::CColor{86,139,173,255});
   c->drawRect({230,100,1050,605},VSTGUI::kDrawStroked);
   c->setFont(VSTGUI::kNormalFont,18);c->setFontColor(letters);
   c->drawString("SAMPLE EDITOR",{260,114,1000,148});
   // Wide, filled peak envelope: adjacent interpolated columns preserve
   // transients without inventing audio or filtering source PCM.
   c->setFillColor(VSTGUI::CColor{5,19,36,255});
   c->drawRect(editorRect(UiWaveEditorLayout::waveform),VSTGUI::kDrawFilled);
   c->setFrameColor(VSTGUI::CColor{47,109,154,255});
   c->drawRect(editorRect(UiWaveEditorLayout::waveform),VSTGUI::kDrawStroked);
   c->setFrameColor(VSTGUI::CColor{47,109,154,255});c->drawLine({284,268},{994,268});
   if(sampleEditorStereo_)c->drawLine({284,326},{994,326});
   const auto n=sampleEditorWave_.size();
   for(int pixel=0;pixel<710;++pixel){
     const double scaled=(static_cast<double>(pixel)+0.5)*n/710.;
     const auto idx=std::min(n-1,static_cast<std::size_t>(scaled));
     const auto next=std::min(n-1,idx+1);
     const double fraction=scaled-static_cast<double>(idx);
     const auto& a=sampleEditorWave_[idx];const auto& b=sampleEditorWave_[next];
     // Include both adjacent ranges: interpolation never loses a one-bin peak.
     const double lo=std::min({static_cast<double>(a.minimum),
           static_cast<double>(b.minimum),0.});
     const double hi=std::max({static_cast<double>(a.maximum),
           static_cast<double>(b.maximum),0.});
     const double mid=sampleEditorStereo_?213.:268.;
     const double amp=sampleEditorStereo_?48.:104.;
     const double sourceFrame=sampleEditorFirst_+
         (pixel+0.5)*static_cast<double>(sampleEditorLast_-sampleEditorFirst_)/710.;
     const auto& e=sampleEditorEdit_;
     // The same discrete source-frame envelope as the audio engine.
     // A one-frame Fade Out now displays silence, matching rendered PCM.
     const double fade=static_cast<double>(e.amplitude(
           static_cast<std::uint32_t>(std::floor(sourceFrame))));
     const double upper=std::clamp(mid-hi*amp*fade,sampleEditorStereo_?156.:156.,
                                   sampleEditorStereo_?264.:381.);
     const double lower=std::clamp(mid-lo*amp*fade,sampleEditorStereo_?156.:156.,
                                   sampleEditorStereo_?264.:381.);
     c->setFrameColor(accent);c->drawLine({284.+pixel,upper},{284.+pixel,lower});
     if(sampleEditorStereo_){
       const auto& rightA=sampleEditorWaveRight_[idx];
       const auto& rightB=sampleEditorWaveRight_[next];
       const double rlo=std::min({static_cast<double>(rightA.minimum),
                                static_cast<double>(rightB.minimum),0.});
       const double rhi=std::max({static_cast<double>(rightA.maximum),
                                static_cast<double>(rightB.maximum),0.});
       c->drawLine({284.+pixel,std::clamp(326.-rhi*48.*fade,273.,381.)},
                   {284.+pixel,std::clamp(326.-rlo*48.*fade,273.,381.)});
     }
     (void)fraction;
   }
   const auto visible=static_cast<double>(sampleEditorLast_-sampleEditorFirst_);
   const auto sx=284.+710.*(static_cast<double>(sampleEditorEdit_.start)-sampleEditorFirst_)/visible;
   const auto ex=284.+710.*(static_cast<double>(sampleEditorEdit_.end)-sampleEditorFirst_)/visible;
   c->setFrameColor(on);c->setLineWidth(2.);
   if(sx>=284.&&sx<=994.)c->drawLine({sx,175},{sx,382});
   c->setFrameColor(letters);
   if(ex>=284.&&ex<=994.)c->drawLine({ex,175},{ex,382});
   // Fade handles and ramp overlays use source-frame positions.
   const auto fadeInX=sx+710.*static_cast<double>(sampleEditorEdit_.fadeIn)/visible;
   const auto fadeOutX=ex-710.*static_cast<double>(sampleEditorEdit_.fadeOut)/visible;
   c->setFrameColor(on);
   if(fadeInX>=284.&&sx<=994.)
     c->drawLine({std::clamp(sx,284.,994.),278},
                 {std::clamp(fadeInX,284.,994.),180});
   c->setFrameColor(letters);
   if(ex>=284.&&fadeOutX<=994.)
     c->drawLine({std::clamp(fadeOutX,284.,994.),180},
                 {std::clamp(ex,284.,994.),278});
   if(fadeInX>=284.&&fadeInX<=994.){
     const auto handle=std::clamp(fadeInX,289.,989.);
     c->setFillColor(on);c->drawRect({handle-5,175,handle+5,188},VSTGUI::kDrawFilled);
   }
   if(fadeOutX>=284.&&fadeOutX<=994.){
     const auto handle=std::clamp(fadeOutX,289.,989.);
     c->setFillColor(letters);c->drawRect({handle-5,175,handle+5,188},VSTGUI::kDrawFilled);
   }
   c->setLineWidth(1.);
   // Scrollbar is proportional to the visible source-frame interval.
   constexpr double scrollX=284.,scrollWidth=710.;
   c->setFillColor(off);c->drawRect({scrollX,387,scrollX+scrollWidth,402},VSTGUI::kDrawFilled);
   const double thumbWidth=scrollWidth*
       static_cast<double>(sampleEditorLast_-sampleEditorFirst_)/sampleEditorFrames_;
   const double thumbX=scrollX+scrollWidth*
       static_cast<double>(sampleEditorFirst_)/sampleEditorFrames_;
   c->setFillColor(accent);
   c->drawRect({thumbX,387,thumbX+thumbWidth,402},VSTGUI::kDrawFilled);
   char info[180]{};
   std::snprintf(info,sizeof(info),"START %u   END %u   FADE IN %u   FADE OUT %u   GAIN %u%%",
        sampleEditorEdit_.start,sampleEditorEdit_.end,
        sampleEditorEdit_.fadeIn,sampleEditorEdit_.fadeOut,
        static_cast<unsigned>(std::lround(sampleEditorEdit_.gain*100.f)));
   c->setFont(VSTGUI::kNormalFont,12);c->setFontColor(letters);
   c->drawString(info,{270,508,1020,532});
   // All eight controls use the same rect for paint, hit-test and press.
   drawControl(c,editorRect(UiWaveEditorLayout::zoomIn),"ZOOM +");
   drawControl(c,editorRect(UiWaveEditorLayout::zoomOut),"ZOOM -");
   drawControl(c,editorRect(UiWaveEditorLayout::fit),"FIT");
   drawControl(c,editorRect(UiWaveEditorLayout::preview),"PREVIEW");
   drawControl(c,editorRect(UiWaveEditorLayout::stop),"STOP");
   drawControl(c,editorRect(UiWaveEditorLayout::reset),"RESET");
   c->setFrameColor(accent);
   c->drawRect(editorRect(UiWaveEditorLayout::previewGroup),VSTGUI::kDrawStroked);
   drawControl(c,editorRect(UiWaveEditorLayout::cancel),"CANCEL");
   drawControl(c,editorRect(UiWaveEditorLayout::apply),"APPLY");
   if(sampleEditorPressed_>=0&&
      sampleEditorPressed_<static_cast<int>(UiWaveEditorLayout::toolbar.size())){
     c->setFrameColor(on);c->setLineWidth(2.);
     c->drawRect(editorRect(UiWaveEditorLayout::toolbar[
       static_cast<std::size_t>(sampleEditorPressed_)]),VSTGUI::kDrawStroked);
     c->setLineWidth(1.);
   }
   // Gain below the signal, compact and directly readable at its control.
   c->drawString("GAIN",{558,405,622,428});
   c->setFillColor(off);c->drawRect({615,416,900,430},VSTGUI::kDrawFilled);
   const auto gainX=615.+285.*std::clamp(static_cast<double>(sampleEditorEdit_.gain)/4.,0.,1.);
   c->setFillColor(accent);c->drawRect({615,416,gainX,430},VSTGUI::kDrawFilled);
   c->setFillColor(letters);c->drawRect({gainX-4,409,gainX+4,437},VSTGUI::kDrawFilled);
   char gainText[36]{};
   std::snprintf(gainText,sizeof(gainText),"%u%%",
                 static_cast<unsigned>(std::lround(sampleEditorEdit_.gain*100.f)));
   c->drawString(gainText,{911,405,1001,439});
   c->setFont(VSTGUI::kNormalFont,12);c->setFontColor(letters);
   if(!status_.empty())
     c->drawString(status_.c_str(),{265,556,1020,585});
 }
 // Decorative corners only; no hit regions or interactive widgets are added.
 drawPanelScrew(c,screwBitmap_,10.,10.,-15.);
 drawPanelScrew(c,screwBitmap_,1270.,10.,25.);
 drawPanelScrew(c,screwBitmap_,10.,fullHeight-10.,40.);
 drawPanelScrew(c,screwBitmap_,1270.,fullHeight-10.,-30.);
 setDirty(false);
}
bool DrumatorView::loadPath(std::size_t row,const std::string& path) {
 const auto outcome=controller_.loadWav(row,path);
 switch(outcome) {
 case Controller::LoadStatus::Accepted:
     status_="SAMPLE LOADED - AUDIO ACTIVE ON NEXT BLOCK";break;
 case Controller::LoadStatus::InvalidRow:
     status_="ERROR: INVALID INSTRUMENT ROW";break;
 case Controller::LoadStatus::NotFound:
     status_="FILE NOT FOUND";break;
 case Controller::LoadStatus::TooLarge:
     status_="SAMPLE TOO LARGE (MAX 16 MiB)";break;
 case Controller::LoadStatus::ReadError:
     status_="SAMPLE READ FAILED";break;
 case Controller::LoadStatus::Unsupported:
     status_="SUPPORTED: WAV PCM/FLOAT AND FLAC, MONO/STEREO";break;
 case Controller::LoadStatus::NoProcessorConnection:
     status_="HOST DID NOT CONNECT CONTROLLER AND PROCESSOR";break;
 case Controller::LoadStatus::Rejected:
     status_="PROCESSOR REJECTED SAMPLE";break;
 }
 invalid();
 return outcome==Controller::LoadStatus::Accepted;
}
bool DrumatorView::acceptFileDrop(VSTGUI::DragEventData data) {
 if(!data.drag) {
   status_="D&D ERROR: NO HOST DATA";invalid();return false;
 }
 const auto position=data.pos;
 if(!sampleDropTargetAt(position.x,position.y)) {
   status_="D&D ERROR: OUTSIDE INSTRUMENT ROWS";
   invalid();return false;
 }
 const auto row=firstRow_+UiCompactLayout::rowAt(position.y,firstRow_,displayedRows(),velocityExpanded_);
 if(row>=kRows) {
   status_="D&D ERROR: INVALID ROW";invalid();return false;
 }
 std::string path;
 if(!DrumatorDropTarget::extractSamplePath(data.drag,path)) {
   status_="D&D ERROR: NO WAV/FLAC PATH";
   invalid();return false;
 }
 return loadPath(row,path);
}

void DrumatorView::load(std::size_t row) {
 auto* frame=getFrame();
 if(!frame)return;
 auto* picker=VSTGUI::CNewFileSelector::create(frame,
                                            VSTGUI::CNewFileSelector::kSelectFile);
 if(!picker) {status_="FILE PICKER UNAVAILABLE";invalid();return;}
 picker->addFileExtension(VSTGUI::CFileExtension("WAV PCM/FLOAT","wav","audio/wav"));
 picker->addFileExtension(VSTGUI::CFileExtension("FLAC","flac","audio/flac"));
 picker->setTitle("125A Drumator - Select WAV/FLAC sample");
 const bool selected=picker->runModal();
 if(selected&&picker->getNumSelectedFiles()>0) {
   const auto* file=picker->getSelectedFile(0);
   if(file)loadPath(row,file);
 }
 picker->forget();
 invalid();
}

bool DrumatorView::openSampleEditor(std::size_t row) {
 SampleEdit edit;std::uint32_t frames=0;
 std::array<WaveformRange,256> wave{};
 // Verify a usable source BEFORE any host resize. Opening an empty row
 // must not enlarge the entire plug-in window.
 if(!controller_.sampleEditorSnapshot(row,edit,frames,wave,0,0,
                                      &sampleEditorRevision_,&sampleEditorWaveRight_,
                                      &sampleEditorStereo_)||frames==0){
   status_="NO SAMPLE TO EDIT";
   invalid();return false;
 }
 const auto scale=static_cast<const DrumatorEditor&>(editor_).effectiveScale();
 constexpr double editorHeight=620.;
 if(getViewSize().getHeight()<editorHeight){
   if(!editor_.requestResize({1280.*scale,editorHeight*scale})){
     status_="SAMPLE EDITOR: HOST REJECTED WINDOW HEIGHT";
     invalid();return false;
   }
   if(auto* parent=getParentView())parent->setViewSize({0,0,1280,editorHeight});
   setViewSize({0,0,1280,editorHeight});
 }
 sampleEditorEdit_=edit;sampleEditorFrames_=frames;sampleEditorWave_=wave;
 sampleEditorRow_=row;sampleEditorFirst_=0;sampleEditorLast_=frames;
 sampleEditorDrag_=0;sampleEditorPressed_=-1;sampleEditorOpen_=true;
 invalid();return true;
}
void DrumatorView::setSampleEditorGain(double x) {
 if(!sampleEditorOpen_)return;
 const auto value=std::clamp((x-615.)/285.,0.,1.);
 sampleEditorEdit_.gain=static_cast<float>(std::round(value*400.)/100.);
 invalid();
}
bool DrumatorView::zoomSampleEditor(bool in,bool full) {
 if(!sampleEditorOpen_||sampleEditorFrames_==0)return false;
 const auto span=sampleEditorLast_-sampleEditorFirst_;
 auto nextSpan=full?sampleEditorFrames_:
     (in?std::max<std::uint32_t>(1,span/2):
       std::min(sampleEditorFrames_,span>sampleEditorFrames_/2?sampleEditorFrames_:span*2));
 nextSpan=std::min(nextSpan,sampleEditorFrames_);
 const auto center=sampleEditorFirst_+span/2;
 const auto first=center>nextSpan/2?center-nextSpan/2:0;
 const auto nextFirst=std::min(first,sampleEditorFrames_-nextSpan);
 auto wave=sampleEditorWave_;
 auto right=sampleEditorWaveRight_;bool stereo=sampleEditorStereo_;
 SampleEdit ignored;std::uint32_t frames=0;
 if(!controller_.sampleEditorSnapshot(sampleEditorRow_,ignored,frames,wave,
                                      nextFirst,nextFirst+nextSpan,nullptr,
                                      &right,&stereo)||
    frames!=sampleEditorFrames_){status_="ZOOM: WAVEFORM UNAVAILABLE";return false;}
 sampleEditorFirst_=nextFirst;sampleEditorLast_=nextFirst+nextSpan;
 sampleEditorWave_=wave;sampleEditorWaveRight_=right;sampleEditorStereo_=stereo;
 invalid();return true;
}
void DrumatorView::moveSampleMarker(double x,bool endMarker) {
 if(!sampleEditorOpen_||sampleEditorFrames_<2)return;
 // Edge-autoscroll while dragging START/END, without mousewheel or modifiers.
 if((x>994.||x<284.)&&sampleEditorLast_>sampleEditorFirst_){
   const auto span=sampleEditorLast_-sampleEditorFirst_;
   const auto step=std::max<std::uint32_t>(1,span/128);
   const auto oldFirst=sampleEditorFirst_;
   const auto nextFirst=x>994.?std::min(sampleEditorFrames_-span,oldFirst+step):
       (oldFirst>step?oldFirst-step:0);
   if(nextFirst!=oldFirst){
     auto wave=sampleEditorWave_;auto right=sampleEditorWaveRight_;
     bool stereo=sampleEditorStereo_;SampleEdit saved;std::uint32_t frames=0;
     if(controller_.sampleEditorSnapshot(sampleEditorRow_,saved,frames,wave,
           nextFirst,nextFirst+span,nullptr,&right,&stereo)&&
           frames==sampleEditorFrames_){
       sampleEditorFirst_=nextFirst;sampleEditorLast_=nextFirst+span;
       sampleEditorWave_=wave;sampleEditorWaveRight_=right;
       sampleEditorStereo_=stereo;
     }
   }
 }
 const auto frame=waveformFrameAt(std::clamp(x,284.,994.)-284.,710.,sampleEditorFirst_,sampleEditorLast_);
 if(endMarker)sampleEditorEdit_.end=std::clamp(frame,
                             sampleEditorEdit_.start+1,sampleEditorFrames_);
 else sampleEditorEdit_.start=std::min(frame,sampleEditorEdit_.end-1);
 const auto span=sampleEditorEdit_.end-sampleEditorEdit_.start;
 sampleEditorEdit_.fadeIn=std::min(sampleEditorEdit_.fadeIn,span);
 sampleEditorEdit_.fadeOut=std::min(sampleEditorEdit_.fadeOut,
                                   span-sampleEditorEdit_.fadeIn);
 invalid();
}

void DrumatorView::moveSampleFade(double x,bool fadeOut) {
 if(!sampleEditorOpen_)return;
 const auto frame=waveformFrameAt(std::clamp(x,284.,994.)-284.,710.,sampleEditorFirst_,sampleEditorLast_);
 const auto& e=sampleEditorEdit_;
 if(fadeOut){
   const auto bounded=std::clamp(frame,e.start+e.fadeIn,e.end);
   sampleEditorEdit_.fadeOut=e.end-bounded;
 }else{
   const auto bounded=std::clamp(frame,e.start,e.end-e.fadeOut);
   sampleEditorEdit_.fadeIn=bounded-e.start;
 }
 invalid();
}
bool DrumatorView::scrollEditorWave(double x) {
 if(!sampleEditorOpen_||!sampleEditorFrames_)return false;
 const auto span=sampleEditorLast_-sampleEditorFirst_;
 if(span>=sampleEditorFrames_)return true;
 const double fraction=std::clamp((x-284.)/710.,0.,1.);
 const auto center=static_cast<std::uint32_t>(
     std::llround(static_cast<double>(sampleEditorFrames_)*fraction));
 const auto start=center>span/2?center-span/2:0u;
 const auto next=std::min(start,sampleEditorFrames_-span);
 if(next==sampleEditorFirst_)return true;
 auto wave=sampleEditorWave_;auto right=sampleEditorWaveRight_;
 bool stereo=sampleEditorStereo_;
 SampleEdit ignored;std::uint32_t count=0;
 if(!controller_.sampleEditorSnapshot(sampleEditorRow_,ignored,count,wave,next,next+span,nullptr,&right,&stereo)||
    count!=sampleEditorFrames_){status_="SCROLL: WAVEFORM UNAVAILABLE";return false;}
 sampleEditorFirst_=next;sampleEditorLast_=next+span;sampleEditorWave_=wave;
 sampleEditorWaveRight_=right;sampleEditorStereo_=stereo;
 invalid();return true;
}
VSTGUI::CMouseEventResult DrumatorView::onMouseDown(
   VSTGUI::CPoint& pos,const VSTGUI::CButtonState& buttons) {
 const double x=pos.x,y=pos.y;
 if(!(buttons&VSTGUI::kLButton))return VSTGUI::kMouseEventNotHandled;
 const auto done=VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;

 if(sampleEditorOpen_) {
   if(UiWaveEditorLayout::scrollbar.hit(x,y)){
     sampleEditorDrag_=3;scrollEditorWave(x);return VSTGUI::kMouseEventHandled;
   }
   if(UiWaveEditorLayout::waveform.hit(x,y)){
     const auto visible=static_cast<double>(sampleEditorLast_-sampleEditorFirst_);
     const auto startX=284.+710.*(static_cast<double>(sampleEditorEdit_.start)-sampleEditorFirst_)/visible;
     const auto endX=284.+710.*(static_cast<double>(sampleEditorEdit_.end)-sampleEditorFirst_)/visible;
     const auto startDistance=std::abs(x-startX);
     const auto endDistance=std::abs(x-endX);
     const auto fadeInX=startX+710.*static_cast<double>(sampleEditorEdit_.fadeIn)/visible;
     const auto fadeOutX=endX-710.*static_cast<double>(sampleEditorEdit_.fadeOut)/visible;
     // Upper handles are for fades; the vertical START/END markers are
     // grabbed anywhere below them. Handles remain draggable at zero fade.
     if(y>=175&&y<=201){
       constexpr double radius=13.;
       const auto inDist=std::abs(x-fadeInX),outDist=std::abs(x-fadeOutX);
       if(inDist<=radius||outDist<=radius){
         sampleEditorDrag_=inDist<=outDist?4:5;
         moveSampleFade(x,sampleEditorDrag_==5);
         return VSTGUI::kMouseEventHandled;
       }
     }
     // Grab either line directly. When neither is near, Shift explicitly
     // selects END; ordinary empty-space clicks continue to position START.
     constexpr double grabRadius=12.;
     if(endDistance<=grabRadius&&(endDistance<=startDistance||startDistance>grabRadius))
       sampleEditorDrag_=2;
     else if(startDistance<=grabRadius)sampleEditorDrag_=1;
     else sampleEditorDrag_=(buttons&VSTGUI::kShift)?2:1;
     moveSampleMarker(x,sampleEditorDrag_==2);
     return VSTGUI::kMouseEventHandled;
   }
   if(UiWaveEditorLayout::gain.hit(x,y)){
     if((buttons&VSTGUI::kControl)||(buttons&VSTGUI::kDoubleClick)){
       sampleEditorEdit_.gain=1.f;sampleEditorDrag_=0;invalid();return done;
     }
     sampleEditorDrag_=6;setSampleEditorGain(x);
     return VSTGUI::kMouseEventHandled;
   }
   if(y>=UiWaveEditorLayout::zoomIn.top&&y<=UiWaveEditorLayout::zoomIn.bottom){
     const int action=UiWaveEditorLayout::buttonAt(x,y);
     sampleEditorPressed_=action;
     switch(action){
       case 0: (void)zoomSampleEditor(true);break;
       case 1: (void)zoomSampleEditor(false);break;
       case 2: (void)zoomSampleEditor(false,true);break;
       case 3:
         if(!controller_.previewSampleEdit(sampleEditorRow_,sampleEditorEdit_,sampleEditorRevision_))
           status_="PREVIEW FAILED";
         else status_="PREVIEW STARTED";
         break;
       case 4:
         status_=controller_.stopSampleEditPreview()?"PREVIEW STOPPED":"PREVIEW STOP FAILED";break;
       case 5:
         sampleEditorEdit_=SampleEdit::reset(sampleEditorFrames_);break;
       case 6:
         if(!controller_.stopSampleEditPreview()){status_="PREVIEW STOP FAILED";break;}
         sampleEditorOpen_=false;sampleEditorDrag_=0;sampleEditorRow_=kRows;
         (void)adjustEditorHeight();break;
       case 7:
         // APPLY is a STOP-then-commit transaction. A failed STOP must not
         // leave a committed edit behind an apparently unfinished modal.
         if(!controller_.stopSampleEditPreview()){
           status_="PREVIEW STOP FAILED; EDIT NOT APPLIED";break;
         }
         if(controller_.commitSampleEdit(sampleEditorRow_,sampleEditorEdit_,sampleEditorRevision_)){
           sampleEditorOpen_=false;sampleEditorDrag_=0;sampleEditorRow_=kRows;
           (void)adjustEditorHeight();
         }else status_="APPLY FAILED: SAMPLE CHANGED OR PROCESSOR REJECTED";
         break;
       default:break;
     }
     invalid();return VSTGUI::kMouseEventHandled;
   }
   return done;
 }
 // Popups are modal: clicking outside dismisses them without changing a step.
 if(popup_!=0){
   const int kind=popup_;
   const auto target=popupRow_;
   popup_=0;popupRow_=kRows;
   bool acted=false,ok=true;
   if(kind==1&&x>=1098&&x<1229&&y>=39&&y<159) {
      const auto option=static_cast<std::size_t>((y-39)/30.);
      if(option<UiCompactLayout::userZooms.size()&&y<39.+option*30.+29.) {
        const auto factor=UiCompactLayout::userZooms[option];
        if(static_cast<DrumatorEditor&>(editor_).setZoomChecked(factor))
          controller_.rememberZoom(factor);
        else {status_="HOST REJECTED ZOOM";ok=false;}
        acted=true;
      }
   }else if(kind==2&&x>=782&&x<968&&y>=86&&y<268) {
      const int idx=static_cast<int>((y-86)/26.);
      if(idx>=0&&idx<7&&y<86.+idx*26.+25.){
        ok=idx==0?controller_.copyCurrentPattern():
            idx==1?controller_.pasteCurrentPattern():
            idx==2?controller_.clearCurrentPattern():
            idx==3?controller_.undoCurrentPattern():
            idx==4?controller_.varyPatternVelocity():
            idx==5?controller_.resetPatternVelocity():
                   controller_.duplicateCurrentPattern();
        acted=true;
      }
   }else if(kind==3&&target<controller_.visibleRows()){
      const auto top=UiCompactLayout::popupTop(rowTopOnPage(target-firstRow_),getViewSize().getHeight(),210.);
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
      const auto top=UiCompactLayout::popupTop(rowTopOnPage(target-firstRow_),getViewSize().getHeight(),145.);
      if(x>=515&&x<615&&y>=top&&y<top+135.){
        const int cell=static_cast<int>((y-top)/27.);
        if(cell>=0&&cell<5&&y<top+cell*27.+26.) {
          ok=controller_.setRowChoke(target,static_cast<unsigned>(cell));acted=true;
        }
      }
   }else if(kind==5&&target<controller_.visibleRows()){
     const auto top=UiCompactLayout::popupTop(rowTopOnPage(target-firstRow_),getViewSize().getHeight(),130.);
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
   if(acted&&!ok)status_="HOST OR PROCESSOR REJECTED SELECTION";
   invalid();return done;
 }
 if(fillMenuRow_>=0) {
   const auto target=static_cast<std::size_t>(fillMenuRow_);
   fillMenuRow_=-1;
   if(target>=firstRow_&&target<firstRow_+displayedRows()){
     const auto top=UiCompactLayout::popupTop(rowTopOnPage(target-firstRow_),getViewSize().getHeight(),230.);
     if(x>=405&&x<614&&y>=top&&y<top+225.){
       const auto selected=static_cast<std::size_t>((y-top)/25.);
       constexpr RowFill fills[]={RowFill::Every2,RowFill::Every4,RowFill::Every8,
           RowFill::Snare24,RowFill::All,RowFill::Clear,RowFill::Offbeat8,
           RowFill::Gallop,RowFill::SnareGhost};
       if(selected<9&&y<top+selected*25.+24.&&
          !controller_.fillRow(target,fills[selected]))
          status_="FILL NOT APPLIED";
     }
   }
   invalid();return done;
 }
 // Explicit pattern-length choice, independent of the operation popup.
 if(y>=8&&y<34&&(UiCompactLayout::hit(x,UiCompactLayout::barOne)||
                  UiCompactLayout::hit(x,UiCompactLayout::barTwo))){
   const bool wantOne=UiCompactLayout::hit(x,UiCompactLayout::barOne);
   const bool isOne=controller_.patternLength(controller_.currentPattern())==16;
   if(wantOne!=isOne&&!controller_.togglePatternLength(controller_.currentPattern()))
     status_="PATTERN LENGTH NOT APPLIED";
   invalid();return done;
 }
 // Global controls: Rotary, direct pattern buttons, Song and two menus.
 if(x>=1120&&x<1250&&y>=7&&y<35){popup_=1;invalid();return done;}
 if(x>=782&&x<901&&y>=52&&y<82){popup_=2;invalid();return done;}
 if(x>=910&&x<991&&y>=52&&y<82) {
   if(!controller_.toggleSongMode())status_="SONG MODE NOT APPLIED";
   invalid();return done;
 }
 if(x>=998&&x<1098&&y>=52&&y<82) {
   songEditorOpen_=!songEditorOpen_;
   // Resize in both directions. If refused, preserve accessible row paging.
   (void)adjustEditorHeight();
   invalid();return done;
 }
 if(y>=kPatternY&&y<kPatternY+30&&x>=kPatternX&&x<kPatternX+8*kPatternWidth){
   const auto idx=static_cast<std::size_t>((x-kPatternX)/kPatternWidth);
   if(x>=kPatternX+idx*kPatternWidth+49.)return VSTGUI::kMouseEventNotHandled;
   const bool ok=(buttons&VSTGUI::kShift)?
      controller_.togglePatternLength(idx):controller_.changePattern(idx);
   if(!ok)status_="PATTERN NOT APPLIED";
   invalid();return done;
 }
 for(int i=4;i<=6;++i){
   const auto center=UiCompactLayout::globalKnobCenters[static_cast<std::size_t>(i-4)];
   if(x>=center-UiCompactLayout::knobRadius&&x<center+UiCompactLayout::knobRadius&&
       y>=76.-UiCompactLayout::knobRadius&&y<76.+UiCompactLayout::knobRadius) {
      const int previous=i==4?static_cast<int>(controller_.swingPercent()):
                         i==5?static_cast<int>(controller_.humanizePercent()):
                              static_cast<int>(controller_.masterVolume());
      if((buttons&VSTGUI::kControl)||(buttons&VSTGUI::kDoubleClick)) {
        if(previous!=UiCompactLayout::defaultValue(i)&&
           !changeKnobValue(i,0,UiCompactLayout::defaultValue(i)))
           status_="CONTROL RESET NOT APPLIED";
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
     if(songPage_>0){
       --songPage_;
       const auto first=UiSongLayout::first(songPage_);
       if(first<count)songSlot_=first;
     }
   }else if(UiSongLayout::nextHit(x,y)){
     if(UiSongLayout::canNext(songPage_,count)){
       ++songPage_;
       const auto first=UiSongLayout::first(songPage_);
       if(first<count)songSlot_=first;
     }
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
     const auto selectedSlot=std::min(songSlot_,count-1);
     const auto current=controller_.songEntry(selectedSlot);
     const bool selectionVisible=songSlot_>=UiSongLayout::first(songPage_)&&
         songSlot_<UiSongLayout::first(songPage_)+UiSongLayout::perPage&&songSlot_<count;
     if(selectionVisible&&x>=115&&x<563) {
       const auto index=static_cast<std::size_t>((x-115)/56.);
       if(index<8&&x<115+index*56.+50.)
         ok=controller_.setSongEntry(selectedSlot,static_cast<std::uint8_t>(index),current.steps);
     }else if(selectionVisible&&x>=678&&x<996) {
       const auto index=static_cast<std::size_t>((x-678)/106.);
       if(index<3&&x<678+index*106.+100.) {
         const std::uint8_t lengths[]={8,16,32};
         ok=controller_.setSongEntry(selectedSlot,current.pattern,lengths[index]);
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
   if(!ok)status_="SONG ENTRY NOT APPLIED";
   invalid();return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
 }

 const double bottom=gridBottom();
 if(!songEditorOpen_&&controller_.visibleRows()>displayedRows()&&
    x>=UiCompactLayout::scrollLeft&&x<UiCompactLayout::scrollRight&&
    y>=UiCompactLayout::scrollTop&&y<bottom-8.) {
   const double top=UiCompactLayout::scrollTop;
   const double length=std::max(1.,bottom-8.-top);
   const double thumb=std::clamp(length*static_cast<double>(displayedRows())/
                      static_cast<double>(controller_.visibleRows()),28.,length);
   const double thumbTop=top+(length-thumb)*static_cast<double>(firstRow_)/
                     static_cast<double>(controller_.visibleRows()-displayedRows());
   scrollGrab_=(y>=thumbTop&&y<=thumbTop+thumb)?y-thumbTop:thumb*.5;
   draggingScroll_=true;(void)moveScroll(y,scrollGrab_);
   return VSTGUI::kMouseEventHandled;
 }
 if(y>=bottom+5&&y<bottom+34){
   if(UiCompactLayout::hit(x,UiCompactLayout::rowAdd)){
     if(controller_.visibleRows()>=kRows)status_="MAXIMUM 16 INSTRUMENTS";
     else {
       const auto count=controller_.visibleRows();
       const auto result=tryRowResizeTransaction(
          [&](){return controller_.addRow();},
          [&](){return adjustEditorHeight();},
          [&](){return controller_.restoreVisibleRows(count);});
       if(result!=RowResizeResult::Applied)status_="COULD NOT ADD ROW";
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
       status_="ONLY AN EMPTY LAST ROW CAN BE REMOVED";
     invalid();return done;
   }
   if(controller_.visibleRows()>displayedRows()&&x>=1068&&x<1229){
     const auto previous=firstRow_;
     const auto stride=std::max<std::size_t>(1,std::min(
         UiCompactLayout::perPage,compactPageRows_));
     if(x<1145)firstRow_=firstRow_>=stride?firstRow_-stride:0;
     else if(x>=1152&&firstRow_+displayedRows()<controller_.visibleRows())
       firstRow_+=stride;
     if(firstRow_!=previous) {
       if(velocityRow_<firstRow_||velocityRow_>=firstRow_+displayedRows())
         velocityRow_=firstRow_;
       closeTransientMenus();
       // Fixed-size hosts still permit the visible, smaller row pages.
       if(!adjustEditorHeight()&&!compactViewport_){
         firstRow_=previous;
         (void)adjustEditorHeight();
       }
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
          status_="STEPS 17-32: SELECT 2 BARS";
          invalid();return done;
        }
        velocityRow_=row;velocityStep_=static_cast<std::size_t>(step);
        // Editing a blank velocity bar must never activate a new note.
        // Notes are created with the dedicated Step buttons above the lane.
        if(controller_.velocity(controller_.currentPattern(),row,velocityStep_)==0){
          status_="EMPTY STEP: ACTIVATE IT IN GRID FIRST";
          invalid();return done;
        }
        laneDragPattern_=controller_.currentPattern();
        laneDragGeneration_=controller_.editGeneration();
        controller_.beginVelocityStroke(laneDragPattern_,this);
        laneDragActive_=true;laneErase_=(buttons&VSTGUI::kControl)!=0;
        if(!controller_.setStepVelocity(laneDragPattern_,row,velocityStep_,
               UiCompactLayout::velocityAt(y,top,laneErase_),this))
          status_="VELOCITY NOT APPLIED";
        invalid();return VSTGUI::kMouseEventHandled;
      }
      return done;
   }
   if(!UiCompactLayout::mainHit(y,top))return done;
   if(x>=16&&x<74) {
     velocityRow_=row;status_="INSTRUMENT SELECTED";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::velocityToggle)&&
      y>=top+70&&y<top+105){
     velocityRow_=row;
     (void)toggleRowVelocity(row);invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::edit)&&y>=top+9&&y<top+40){
      if(controller_.hasSample(row))(void)openSampleEditor(row);
      invalid();return done;
    }
    if(UiCompactLayout::hit(x,UiCompactLayout::sampleMenu)&&
      y>=top+9&&y<top+40){
     popup_=5;popupRow_=row;invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::sample)&&y>=top+9&&y<top+40){
     if(buttons&VSTGUI::kShift)editName(row);
     else if(buttons&VSTGUI::kAlt){
       if(!controller_.clearSample(row))status_="SAMPLE NOT REMOVED";
     }else load(row);
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::previous)&&y>=top+9&&y<top+40){
     if(controller_.stepSample(row,-1)!=Controller::LoadStatus::Accepted)
       status_="NO PREVIOUS SAMPLE";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::next)&&y>=top+9&&y<top+40){
     if(controller_.stepSample(row,1)!=Controller::LoadStatus::Accepted)
       status_="NO NEXT SAMPLE";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::preview)&&y>=top+9&&y<top+40){
     if(!controller_.previewSample(row))status_="SAMPLE PREVIEW FAILED";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::fill)&&y>=top+9&&y<top+40){
     fillMenuRow_=static_cast<int>(row);invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::mute)&&y>=top+9&&y<top+40){
     if(!controller_.toggleRowGate(row,false))status_="MUTE NOT APPLIED";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::solo)&&y>=top+9&&y<top+40){
     if(!controller_.toggleRowGate(row,true))status_="SOLO NOT APPLIED";
     invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::choke)&&y>=top+73&&y<top+105){
     popup_=4;popupRow_=row;invalid();return done;
   }
   if(UiCompactLayout::hit(x,UiCompactLayout::output)&&y>=top+9&&y<top+40){
     popup_=3;popupRow_=row;invalid();return done;
   }
   if(y>=top+70&&y<top+105){
     const auto step=UiCompactLayout::stepAt(x);
     if(step>=0){
       if(static_cast<std::size_t>(step)>=controller_.patternLength(controller_.currentPattern())) {
         status_="STEPS 17-32: SELECT 2 BARS";
         invalid();return done;
       }
       velocityStep_=static_cast<std::size_t>(step);
       const bool ok=(buttons&VSTGUI::kShift)?
         controller_.cycleStepVelocity(controller_.currentPattern(),row,velocityStep_):
         controller_.toggleStep(controller_.currentPattern(),row,velocityStep_);
       if(!ok)status_="STEP NOT APPLIED";
       invalid();return done;
     }
   }
   if(controller_.rowRandomVelocity(row)==0&&x>=448&&x<544&&
      y>=top+73&&y<top+105){
     if(!controller_.randomizeRowVelocity(row))
       status_="RANDOM VELOCITY: NO NOTES OR EDIT REJECTED";
     invalid();return done;
   }
   for(int i=0;i<4;++i){
     if(i==3&&controller_.rowRandomVelocity(row)==0)continue;
     const auto center=UiCompactLayout::rowKnobCenters[static_cast<std::size_t>(i)];
     if(x>=center-UiCompactLayout::knobRadius&&x<center+UiCompactLayout::knobRadius&&
        y>=top+88.-UiCompactLayout::knobRadius&&
        y<top+88.+UiCompactLayout::knobRadius){
       const int previous=i==0?static_cast<int>(controller_.rowGain(row)):
          i==1?controller_.rowPan(row):
          i==2?controller_.rowPitch(row):
               static_cast<int>(controller_.rowRandomVelocity(row));
       if((buttons&VSTGUI::kControl)||(buttons&VSTGUI::kDoubleClick)){
         const auto def=UiCompactLayout::defaultValue(i);
         if(previous!=def&&!changeKnobValue(i,row,def))
           status_="SOUND CONTROL RESET NOT APPLIED";
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
 if(draggingScroll_){
   if(buttons&VSTGUI::kLButton)(void)moveScroll(pos.y,scrollGrab_);
   else draggingScroll_=false;
   return VSTGUI::kMouseEventHandled;
 }
 if(sampleEditorOpen_){
   if((buttons&VSTGUI::kLButton)&&sampleEditorDrag_==6)setSampleEditorGain(pos.x);
   else if((buttons&VSTGUI::kLButton)&&sampleEditorDrag_==3)scrollEditorWave(pos.x);
   else if((buttons&VSTGUI::kLButton)&&sampleEditorDrag_>=4)
     moveSampleFade(pos.x,sampleEditorDrag_==5);
   else if((buttons&VSTGUI::kLButton)&&sampleEditorDrag_)
     moveSampleMarker(pos.x,sampleEditorDrag_==2);
   return VSTGUI::kMouseEventHandled;
 }
 if(activeKnob_>=0){
   if(knobGeneration_!=controller_.editGeneration()||!(buttons&VSTGUI::kLButton)){
     activeKnob_=-1;knobRow_=kRows;invalid();
     return VSTGUI::kMouseEventHandled;
   }
   const auto requested=UiCompactLayout::knobValueForSlot(
        activeKnob_,knobStartY_,pos.y,knobStartValue_,knobFine_);
   if(!changeKnobValue(activeKnob_,knobRow_,requested))
      status_="CONTROL VALUE NOT APPLIED";
   invalid();return VSTGUI::kMouseEventHandled;
 }
 if(!laneDragActive_)return VSTGUI::kMouseEventNotHandled;
 if(laneDragGeneration_!=controller_.editGeneration()||
    laneDragPattern_!=controller_.currentPattern()){
   if(laneDragGeneration_==controller_.editGeneration())
     controller_.endVelocityStroke(false,this);
   laneDragActive_=false;laneDragGeneration_=0;
   return VSTGUI::kMouseEventHandled;
 }
 if(!(buttons&VSTGUI::kLButton)){
   laneDragActive_=false;laneDragGeneration_=0;
   controller_.endVelocityStroke(true,this);
   return VSTGUI::kMouseEventHandled;
 }
 const auto step=UiCompactLayout::stepAt(pos.x);
 if(step<0||static_cast<std::size_t>(step)>=controller_.patternLength(laneDragPattern_)||
    songEditorOpen_||velocityRow_<firstRow_||
    velocityRow_>=firstRow_+displayedRows()||!velocityExpanded_[velocityRow_])
   return VSTGUI::kMouseEventHandled;
 const auto idx=velocityRow_-firstRow_;
 velocityStep_=static_cast<std::size_t>(step);
 // Pointer movement across empty columns must not silently add steps.
 if(controller_.velocity(laneDragPattern_,velocityRow_,velocityStep_)==0)
   return VSTGUI::kMouseEventHandled;
 const auto value=UiCompactLayout::velocityAt(pos.y,rowTopOnPage(idx),laneErase_);
 if(!controller_.setStepVelocity(laneDragPattern_,velocityRow_,velocityStep_,value,this))
   status_="VELOCITY DRAG NOT APPLIED";
 invalid();return VSTGUI::kMouseEventHandled;
}
VSTGUI::CMouseEventResult DrumatorView::onMouseUp(
    VSTGUI::CPoint&,const VSTGUI::CButtonState&) {
 if(draggingScroll_){draggingScroll_=false;return VSTGUI::kMouseEventHandled;}
 if(sampleEditorOpen_){sampleEditorDrag_=0;sampleEditorPressed_=-1;
   invalid();return VSTGUI::kMouseEventHandled;}
 if(activeKnob_>=0){
   activeKnob_=-1;knobRow_=kRows;
   invalid();return VSTGUI::kMouseEventHandled;
 }
 if(!laneDragActive_)return VSTGUI::kMouseEventNotHandled;
 const bool generationValid=laneDragGeneration_==controller_.editGeneration();
 const bool fresh=generationValid&&laneDragPattern_==controller_.currentPattern();
 laneDragActive_=false;laneDragGeneration_=0;
 if(fresh)controller_.endVelocityStroke(true,this);
 else if(generationValid)controller_.endVelocityStroke(false,this);
 return VSTGUI::kMouseEventHandled;
}
VSTGUI::CMouseEventResult DrumatorView::onMouseCancel(){
 sampleEditorDrag_=0;sampleEditorPressed_=-1;
 draggingScroll_=false;
 activeKnob_=-1;knobRow_=kRows;
 detailDragControl_=-1;detailDragRow_=kRows;
 const bool fresh=laneDragActive_&&
                  laneDragGeneration_==controller_.editGeneration();
 laneDragActive_=false;laneDragGeneration_=0;
 if(fresh)controller_.endVelocityStroke(false,this);
 invalid();return VSTGUI::kMouseEventHandled;
}

}
