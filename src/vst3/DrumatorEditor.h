#pragma once
#include "vstgui/plugin-bindings/vst3editor.h"
namespace drumator {
// The pinned VSTGUI exposes the combined host DPI and user zoom factor
// as protected API. Expose it only through our concrete editor subclass.
class DrumatorEditor final : public VSTGUI::VST3Editor {
public:
 using VSTGUI::VST3Editor::VST3Editor;
 double effectiveScale() const {return getAbsScaleFactor();}
};
}
