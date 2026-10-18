#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "core/SequencerCore.h"
#include "core/PatternOperations.h"
#include "core/VisibleRows.h"
#include "core/KitState.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drumator {
class DrumatorView;
class Controller final : public Steinberg::Vst::EditController,
                         public VSTGUI::VST3EditorDelegate {
public:
 static Steinberg::FUnknown* createInstance(void*) {
   return static_cast<Steinberg::Vst::IEditController*>(new Controller());
 }
 Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID,Steinberg::Vst::ParamValue) SMTG_OVERRIDE;
 void registerView(DrumatorView* view);
 void unregisterView(DrumatorView* view);
 Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;
 VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr name,
                  const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
                  VSTGUI::VST3Editor*) override;

 bool clearSample(std::size_t row);
 bool toggleRowGate(std::size_t row,bool solo);
 bool cycleRowChoke(std::size_t row);
 bool cycleRowOutput(std::size_t row);
 unsigned rowOutput(std::size_t row) const noexcept {return row<kRows?output_[row]:0;}
 unsigned rowChoke(std::size_t row) const noexcept {return row<kRows?choke_[row]:0;}
 bool adjustRowMix(std::size_t row,bool adjustPan,int delta);
 bool adjustRowPitch(std::size_t row,int delta);
 int rowPitch(std::size_t row) const noexcept {return row<kRows?static_cast<int>(pitch_[row])-24:0;}
 unsigned rowGain(std::size_t row) const noexcept {return row<kRows?gain_[row]:100;}
 int rowPan(std::size_t row) const noexcept {return row<kRows?static_cast<int>(pan_[row])-100:0;}
 bool isRowMuted(std::size_t row) const noexcept {return row<kRows&&muted_[row];}
 bool isRowSolo(std::size_t row) const noexcept {return row<kRows&&solo_[row];}
 bool toggleStep(std::size_t pattern,std::size_t row,std::size_t step);
 bool cycleStepVelocity(std::size_t pattern,std::size_t row,std::size_t step);
 bool changePattern(std::size_t pattern);
 bool copyCurrentPattern();
 bool pasteCurrentPattern();
 bool duplicateCurrentPattern(bool* selectedCopy=nullptr);
 bool clearCurrentPattern();
 bool fillRow(std::size_t row,RowFill mode);
 bool adjustSwing(int delta);
 bool adjustHumanize(int delta);
 unsigned humanizePercent() const noexcept {return humanizePercent_;}
 unsigned swingPercent() const noexcept {return swingPercent_;}
 bool addRow();
 bool hideLastEmptyRow();
 double preferredZoom() const noexcept {return preferredZoom_;}
 void rememberZoom(double factor) noexcept {preferredZoom_=(factor>1.25)?1.5:1.0;}
 bool restoreVisibleRows(std::size_t previous);
 std::size_t visibleRows() const noexcept {return visibleRows_;}
 bool canHideLast() const noexcept;
 enum class LoadStatus {
  Accepted, InvalidRow, NotFound, TooLarge, ReadError, Unsupported,
  NoProcessorConnection, Rejected
 };
 LoadStatus loadWav(std::size_t row,const std::string& path);
 LoadStatus stepSample(std::size_t row,int direction);
 std::uint8_t velocity(std::size_t pattern,std::size_t row,std::size_t step) const noexcept;
 std::size_t currentPattern() noexcept;
 const std::string& label(std::size_t row) const noexcept;
private:
 void notifyViews(bool relayout);
 std::vector<DrumatorView*> views_;
 Steinberg::tresult sendEditMessage(Steinberg::Vst::IMessage* message);
 std::array<std::uint8_t,kCells> grid_{};
 PatternSteps clipboard_{};
 bool clipboardValid_{false};
 bool replacePattern(std::size_t p,const PatternSteps& data);
 std::array<std::string,kRows> labels_{};
 // Filesystem navigation is session-only. Embedded WAV data remains self-contained.
 std::array<std::string,kRows> samplePaths_{};
 std::array<bool,kRows> muted_{};
 std::array<bool,kRows> solo_{};
 std::array<std::uint8_t,kRows> gain_{};
 std::array<std::uint8_t,kRows> pan_{};
 std::array<std::uint8_t,kRows> pitch_{};
 std::array<std::uint8_t,kRows> choke_{};
 std::array<std::uint8_t,kRows> output_{};
 std::uint8_t visibleRows_{kDefaultVisibleRows};
 double preferredZoom_{1.0};
 std::uint8_t swingPercent_{0};
 std::uint8_t humanizePercent_{0};
};
}
