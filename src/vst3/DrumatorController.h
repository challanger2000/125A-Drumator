#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "core/SequencerCore.h"
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

 bool toggleStep(std::size_t pattern,std::size_t row,std::size_t step);
 bool changePattern(std::size_t pattern);
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
};
}
