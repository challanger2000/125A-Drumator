#pragma once
#include <array>
namespace drumator {
// One source of truth for the eight Wave-Editor toolbar buttons.
struct UiWaveEditorLayout final {
 struct Rect {
   double left,top,right,bottom;
   constexpr bool hit(double x,double y) const noexcept {
     return x>=left&&x<=right&&y>=top&&y<=bottom;
   }
   constexpr double width() const noexcept {return right-left;}
 };
 static constexpr Rect waveform{284.,175.,994.,382.};
 static constexpr Rect scrollbar{284.,385.,994.,405.};
 static constexpr Rect gain{615.,405.,900.,442.};
 static constexpr Rect zoomIn{265.,455.,345.,491.};
 static constexpr Rect zoomOut{349.,455.,429.,491.};
 static constexpr Rect fit{433.,455.,513.,491.};
 static constexpr Rect preview{529.,455.,621.,491.};
 static constexpr Rect stop{622.,455.,714.,491.};
 static constexpr Rect reset{731.,455.,811.,491.};
 static constexpr Rect cancel{835.,455.,925.,491.};
 static constexpr Rect apply{929.,455.,1019.,491.};
 static constexpr Rect previewGroup{527.,453.,716.,493.};
 // Order: zoom in, zoom out, fit, preview, stop, reset, cancel, apply.
 static constexpr std::array<Rect,8> toolbar{{zoomIn,zoomOut,fit,preview,stop,reset,cancel,apply}};
 static constexpr int buttonAt(double x,double y) noexcept {
   for(int i=0;i<static_cast<int>(toolbar.size());++i)
     if(toolbar[static_cast<std::size_t>(i)].hit(x,y))return i;
   return -1;
 }
};
static_assert(UiWaveEditorLayout::zoomIn.width()==UiWaveEditorLayout::zoomOut.width());
static_assert(UiWaveEditorLayout::zoomIn.width()==UiWaveEditorLayout::fit.width());
static_assert(UiWaveEditorLayout::zoomIn.width()==UiWaveEditorLayout::reset.width());
static_assert(UiWaveEditorLayout::preview.width()==UiWaveEditorLayout::stop.width());
static_assert(UiWaveEditorLayout::cancel.width()==UiWaveEditorLayout::apply.width());
static_assert(UiWaveEditorLayout::preview.right<UiWaveEditorLayout::stop.left);
}
