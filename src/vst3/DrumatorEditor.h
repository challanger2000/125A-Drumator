#pragma once
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/cframe.h"
#include <cmath>
namespace drumator {
// The pinned VSTGUI exposes the combined host DPI and user zoom factor
// as protected API. Expose it only through our concrete editor subclass.
class DrumatorEditor final : public VSTGUI::VST3Editor {
public:
 using VSTGUI::VST3Editor::VST3Editor;
 double effectiveScale() const {return getAbsScaleFactor();}
 // VSTGUI updates zoomFactor before negotiating a host resize. On rejection
 // its frame restores the old transform but the editor value remains stale.
 bool setZoomChecked(double factor) {
   const double previous=getZoomFactor();
   setZoomFactor(factor);
   if(const auto* activeFrame=getFrame()) {
     if(std::abs(activeFrame->getZoom()-getAbsScaleFactor())>0.001) {
       zoomFactor=previous;
       return false;
     }
   }
   return true;
 }
};
}
