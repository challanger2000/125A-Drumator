#pragma once
#include "vstgui/lib/cview.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include "vstgui/lib/dragging.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <string>
#include "core/SequencerCore.h"
namespace VSTGUI {class CTextEdit;}
namespace drumator {
class Controller;
class DrumatorView final : public VSTGUI::CView, public VSTGUI::IDropTarget,
                           public VSTGUI::IControlListener {
public:
 ~DrumatorView() override;
 void refreshFromHost(bool relayout);
 DrumatorView(const VSTGUI::CRect& size,Controller& controller,
              VSTGUI::VST3Editor& editor);
 void draw(VSTGUI::CDrawContext* context) override;
 bool attached(VSTGUI::CView* parent) override;
 bool removed(VSTGUI::CView* parent) override;
 void valueChanged(VSTGUI::CControl* control) override;
 VSTGUI::CMouseEventResult onMouseDown(
     VSTGUI::CPoint& point,const VSTGUI::CButtonState& buttons) override;
 VSTGUI::SharedPointer<VSTGUI::IDropTarget> getDropTarget() override {return this;}
 VSTGUI::DragOperation onDragEnter(VSTGUI::DragEventData data) override;
 VSTGUI::DragOperation onDragMove(VSTGUI::DragEventData data) override;
 void onDragLeave(VSTGUI::DragEventData data) override;
 bool onDrop(VSTGUI::DragEventData data) override;
 bool acceptFileDrop(VSTGUI::DragEventData data);
 std::size_t visibleRowsForHit() const noexcept;
private:
 Controller& controller_;
 VSTGUI::CTextEdit* nameEdit_{nullptr};
 std::size_t editingNameRow_{kRows};
 void editName(std::size_t row);
 VSTGUI::VST3Editor& editor_;
 int fillMenuRow_{-1};
 std::size_t velocityRow_{0};
 std::size_t velocityStep_{0};
 std::size_t firstRow_{0};
 std::size_t songSlot_{0};
 bool songEditorOpen_{false};
 bool compactViewport_{false};
 std::size_t displayedRows() const noexcept;
 std::string status_{"LEER: WAV KLICKEN ODER DATEI AUF ZEILE ZIEHEN"};
 void load(std::size_t row);
 bool adjustEditorHeight();
 bool loadPath(std::size_t row,const std::string& path);
};
}
