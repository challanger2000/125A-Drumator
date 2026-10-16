#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "core/SequencerCore.h"
#include "core/VisibleRows.h"
#include "core/KitState.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace drumator {
class Controller final : public Steinberg::Vst::EditController,
                         public VSTGUI::VST3EditorDelegate {
public:
 static Steinberg::FUnknown* createInstance(void*) {
   return static_cast<Steinberg::Vst::IEditController*>(new Controller());
 }
 Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) SMTG_OVERRIDE;
 Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream*) SMTG_OVERRIDE;
 Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;
 VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr name,
                  const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
                  VSTGUI::VST3Editor*) override;

 bool toggleRowGate(std::size_t row,bool solo);
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
 bool addRow();
 bool hideLastEmptyRow();
 std::size_t visibleRows() const noexcept {return visibleRows_;}
 bool canHideLast() const noexcept;
 enum class LoadStatus {
  Accepted, InvalidRow, NotFound, TooLarge, ReadError, Unsupported,
  NoProcessorConnection, Rejected
 };
 LoadStatus loadWav(std::size_t row,const std::string& path);
 std::uint8_t velocity(std::size_t pattern,std::size_t row,std::size_t step) const noexcept;
 std::size_t currentPattern() noexcept;
 const std::string& label(std::size_t row) const noexcept;
private:
 std::array<std::uint8_t,kCells> grid_{};
 std::array<std::string,kRows> labels_{};
 std::array<bool,kRows> muted_{};
 std::array<bool,kRows> solo_{};
 std::array<std::uint8_t,kRows> gain_{};
 std::array<std::uint8_t,kRows> pan_{};
 std::array<std::uint8_t,kRows> pitch_{};
 std::uint8_t visibleRows_{kDefaultVisibleRows};
};
}
