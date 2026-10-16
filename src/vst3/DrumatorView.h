#pragma once
#include "vstgui/lib/cview.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <string>
namespace drumator {
class Controller;
class DrumatorView final : public VSTGUI::CView {
public:
 DrumatorView(const VSTGUI::CRect& size,Controller& controller,
              VSTGUI::VST3Editor& editor);
 void draw(VSTGUI::CDrawContext* context) override;
 VSTGUI::CMouseEventResult onMouseDown(
     VSTGUI::CPoint& point,const VSTGUI::CButtonState& buttons) override;
private:
 Controller& controller_;
 VSTGUI::VST3Editor& editor_;
 std::string status_{"WAV PCM16 MONO/STEREO  |  CLICK ROW TO LOAD"};
 void load(std::size_t row);
};
}
