#pragma once
#include "vstgui/lib/cview.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include "vstgui/lib/controls/itexteditlistener.h"
#include "vstgui/lib/dragging.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <string>
#include <array>
#include "core/SequencerCore.h"
#include "core/SampleEdit.h"
#include <cstdint>
namespace VSTGUI {class CTextEdit; class CBitmap;}
namespace drumator {
class Controller;
class DrumatorView final : public VSTGUI::CView, public VSTGUI::IDropTarget,
                           public VSTGUI::IControlListener, public VSTGUI::ITextEditListener {
public:
 ~DrumatorView() override;
 void refreshFromHost(bool relayout);
 void discardTransientEditsForRecall() noexcept;
 void finishTransientEditsAfterRecall();
 double currentZoom() const;
 bool zoomMatches(double factor) const;
 DrumatorView(const VSTGUI::CRect& size,Controller& controller,
              VSTGUI::VST3Editor& editor);
 void draw(VSTGUI::CDrawContext* context) override;
 bool attached(VSTGUI::CView* parent) override;
 bool removed(VSTGUI::CView* parent) override;
 void valueChanged(VSTGUI::CControl* control) override;
 void onTextEditPlatformControlTookFocus(VSTGUI::CTextEdit*) override {}
 void onTextEditPlatformControlLostFocus(VSTGUI::CTextEdit*) override;
 bool applyZoomFromState(double factor);
 VSTGUI::CMouseEventResult onMouseDown(
     VSTGUI::CPoint& point,const VSTGUI::CButtonState& buttons) override;
 VSTGUI::CMouseEventResult onMouseMoved(
     VSTGUI::CPoint& point,const VSTGUI::CButtonState& buttons) override;
 VSTGUI::CMouseEventResult onMouseUp(
     VSTGUI::CPoint& point,const VSTGUI::CButtonState& buttons) override;
 VSTGUI::CMouseEventResult onMouseCancel() override;
 VSTGUI::SharedPointer<VSTGUI::IDropTarget> getDropTarget() override {return this;}
 VSTGUI::DragOperation onDragEnter(VSTGUI::DragEventData data) override;
 VSTGUI::DragOperation onDragMove(VSTGUI::DragEventData data) override;
 void onDragLeave(VSTGUI::DragEventData data) override;
 bool onDrop(VSTGUI::DragEventData data) override;
 bool acceptFileDrop(VSTGUI::DragEventData data);
 std::size_t visibleRowsForHit() const noexcept;
 bool sampleDropTargetAt(double x,double y) const noexcept;
private:
 Controller& controller_;
 VSTGUI::CBitmap* backgroundBitmap_{nullptr};
 VSTGUI::CBitmap* productLogoBitmap_{nullptr};
 VSTGUI::CBitmap* knobBitmap_{nullptr};
 VSTGUI::CBitmap* screwBitmap_{nullptr};
 VSTGUI::CTextEdit* nameEdit_{nullptr};
 std::size_t editingNameRow_{kRows};
 std::uint64_t nameEditGeneration_{0};
 void editName(std::size_t row);
 VSTGUI::VST3Editor& editor_;
 int fillMenuRow_{-1};
 std::size_t velocityRow_{0};
 std::size_t velocityStep_{0};
 std::size_t firstRow_{0};
 std::size_t songSlot_{0};
 std::size_t songPage_{0};
 bool songEditorOpen_{false};
 // UI-only: each instrument controls its own Velocity lane.
 std::array<bool,kRows> velocityExpanded_{};
 // 0=no popup, 1=Zoom, 2=Pattern actions, 3=Output, 4=Choke, 5=Sample actions.
 int popup_{0};
 bool sampleEditorOpen_{false};
 std::size_t sampleEditorRow_{kRows};
 SampleEdit sampleEditorEdit_{};
 std::uint32_t sampleEditorRevision_{0};
 std::uint32_t sampleEditorFrames_{0};
 std::uint32_t sampleEditorFirst_{0},sampleEditorLast_{0};
 std::array<WaveformRange,256> sampleEditorWave_{};
 std::array<WaveformRange,256> sampleEditorWaveRight_{};
 bool sampleEditorStereo_{false};
 int sampleEditorPressed_{-1}; // transient pressed toolbar cell; UI only
 int sampleEditorDrag_{0}; // 1=start, 2=end, 3=scroll, 4=fade-in, 5=fade-out, 6=gain
 void setSampleEditorGain(double x);
 bool zoomSampleEditor(bool in,bool full=false);
 void moveSampleFade(double x,bool fadeOut);
 bool scrollEditorWave(double x);
 std::size_t popupRow_{kRows};
 int activeKnob_{-1}; // 0..3 row, 4=Swing,5=Humanize,6=Master
 std::size_t knobRow_{kRows};
 double knobStartY_{0.};
 int knobStartValue_{0};
 bool knobFine_{false};
 std::uint64_t knobGeneration_{0};
 bool laneDragActive_{false};
 std::size_t laneDragPattern_{0};
 std::uint64_t laneDragGeneration_{0};
 bool laneErase_{false};
 int detailDragControl_{-1};
 std::size_t detailDragRow_{kRows};
 bool changeDetailValue(std::size_t row,int slot,int requested);
 bool compactViewport_{false};
 bool draggingScroll_{false};
 double scrollGrab_{0.};
 bool moveScroll(double y,double grab);
 std::size_t compactPageRows_{4};
 std::size_t displayedRows() const noexcept;
 double rowTopOnPage(std::size_t pageIndex) const noexcept;
 double gridBottom() const noexcept;
 bool toggleRowVelocity(std::size_t row);
 bool changeKnobValue(int slot,std::size_t row,int value);
 void closeTransientMenus() noexcept;

 std::string status_{"EMPTY: CLICK SAMPLE OR DROP A WAV/FLAC"};
 void load(std::size_t row);
 bool openSampleEditor(std::size_t row);
 void moveSampleMarker(double x,bool endMarker);
 bool adjustEditorHeight();
 bool loadPath(std::size_t row,const std::string& path);
};
}
