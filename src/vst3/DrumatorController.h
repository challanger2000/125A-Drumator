#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "core/SequencerCore.h"
#include "core/PatternOperations.h"
#include "core/PatternUndo.h"
#include "core/KitEditGeneration.h"
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
 KitEditGeneration::Token editGeneration() const noexcept {return editGeneration_.current();}
 Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;
 VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr name,
                  const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
                  VSTGUI::VST3Editor*) override;

 bool clearSample(std::size_t row);
 bool previewSample(std::size_t row);
 bool toggleRowGate(std::size_t row,bool solo);
 bool cycleRowChoke(std::size_t row,int direction=1);
 bool setRowChoke(std::size_t row,unsigned group);
 bool cycleRowOutput(std::size_t row);
 bool setRowOutput(std::size_t row,unsigned bus);
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
 bool setStepVelocity(std::size_t pattern,std::size_t row,std::size_t step,std::uint8_t velocity);
 void beginVelocityStroke(std::size_t p);
 void endVelocityStroke(bool commit);
 bool changePattern(std::size_t pattern);
 bool togglePatternLength(std::size_t pattern);
 std::uint8_t patternLength(std::size_t p) const noexcept {return p<kPatterns?patternLengths_[p]:32;}
 bool configureSong(bool enabled,std::size_t count,const SongChain& chain);
 bool toggleSongMode();
 bool editSongEntry(std::size_t slot,int patternDelta,int lengthDelta);
 bool setSongEntry(std::size_t slot,std::uint8_t pattern,std::uint8_t steps);
 bool adjustSongCount(int delta);
 bool songEnabled() const noexcept {return songEnabled_;}
 std::size_t songCount() const noexcept {return songChain_.count();}
 SongChain::Entry songEntry(std::size_t slot) const noexcept {return songChain_.entry(slot);}
 bool copyCurrentPattern();
 bool pasteCurrentPattern();
 bool canDuplicateCurrentPattern() noexcept;
 bool duplicateCurrentPattern(bool* selectedCopy=nullptr);
 bool clearCurrentPattern();
 bool undoCurrentPattern();
 bool canUndoCurrentPattern() noexcept {return undo_.available(currentPattern());}
 // One-shot, per-instrument edit for the forthcoming RANDOM VEL button.
 // Modifies only occupied steps in the active pattern/active bar length,
 // persisted through PatternReplace as one undoable operation.
 bool randomizeRowVelocity(std::size_t row);
 bool varyPatternVelocity();
 bool resetPatternVelocity();
 std::uint32_t velocityVariation_{0};
 bool fillRow(std::size_t row,RowFill mode);
 bool adjustSwing(int delta);
 bool adjustHumanize(int delta);
 bool adjustMasterVolume(int delta);
 unsigned masterVolume() const noexcept {return masterVolume_;}
 bool adjustRandomVelocity(std::size_t row,int delta);
 unsigned rowRandomVelocity(std::size_t row) const noexcept {
   return row<kRows?randomVelocity_[row]:0;
 }
 unsigned humanizePercent() const noexcept {return humanizePercent_;}
 unsigned swingPercent() const noexcept {return swingPercent_;}
 bool addRow();
 bool hideLastEmptyRow();
 double preferredZoom() const noexcept {return preferredZoom_;}
 void rememberZoom(double factor) noexcept {
   preferredZoom_=factor==1.0||factor==1.2||factor==1.3||factor==1.5?factor:1.0;
 }
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
 bool renameRow(std::size_t row,const std::string& name);
 const std::string& customName(std::size_t row) const noexcept {
   static const std::string empty;
   return row<kRows?customNames_[row]:empty;
 }
private:
 void notifyViews(bool relayout);
 void invalidateTransientEditsForRecall() noexcept;
 std::vector<DrumatorView*> views_;
 Steinberg::tresult sendEditMessage(Steinberg::Vst::IMessage* message);
 std::array<std::uint8_t,kCells> grid_{};
 std::array<std::uint8_t,kPatterns> patternLengths_{{32,32,32,32,32,32,32,32}};
 PatternSteps clipboard_{};
 PatternUndo undo_{};
 PatternSteps strokeBefore_{};
 KitEditGeneration editGeneration_{};
 KitEditGeneration::Token strokeGeneration_{0};
 std::size_t strokePattern_{kPatterns};
 bool clipboardValid_{false};
 bool replacePattern(std::size_t p,const PatternSteps& data,bool remember=true);
 void rememberUndoStep(std::size_t p,std::size_t r,std::size_t s,std::uint8_t next);
 std::array<std::string,kRows> labels_{};
 std::array<std::string,kRows> customNames_{};
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
 std::uint8_t masterVolume_{100};
 std::array<std::uint8_t,kRows> randomVelocity_{};
 SongChain songChain_{};
 bool songEnabled_{false};
};
}
