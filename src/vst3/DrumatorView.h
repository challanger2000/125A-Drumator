#pragma once
#include "vstgui/lib/cview.h"
#include "vstgui/lib/dragging.h"
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
 bool acceptFileDrop(VSTGUI::DragEventData data);
private:
 Controller& controller_;
 VSTGUI::VST3Editor& editor_;
 std::string status_{"LEER: WAV KLICKEN ODER DATEI AUF ZEILE ZIEHEN"};
 void load(std::size_t row);
 void loadPath(std::size_t row,const std::string& path);
};
}
