#pragma once
#include "vstgui/lib/cview.h"
#include "vstgui/lib/dragging.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <string>
namespace drumator {
class Controller;
class DrumatorView final : public VSTGUI::CView {
public:
 ~DrumatorView() override;
 void refreshFromHost(bool relayout);
 DrumatorView(const VSTGUI::CRect& size,Controller& controller,
              VSTGUI::VST3Editor& editor);
 void draw(VSTGUI::CDrawContext* context) override;
 bool attached(VSTGUI::CView* parent) override;
 VSTGUI::CMouseEventResult onMouseDown(
     VSTGUI::CPoint& point,const VSTGUI::CButtonState& buttons) override;
 bool acceptFileDrop(VSTGUI::DragEventData data);
 std::size_t visibleRowsForHit() const noexcept;
private:
 Controller& controller_;
 VSTGUI::VST3Editor& editor_;
 int fillMenuRow_{-1};
 std::string status_{"LEER: WAV KLICKEN ODER DATEI AUF ZEILE ZIEHEN"};
 void load(std::size_t row);
 bool adjustEditorHeight();
 void loadPath(std::size_t row,const std::string& path);
};
}
