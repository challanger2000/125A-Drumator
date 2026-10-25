#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "core/SequencerCore.h"
#include "core/SampleEdit.h"
#include "core/WavPcm16Decoder.h"
#include "core/WaveformExtremaIndex.h"
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
 ~Controller() override;
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

 bool hasSample(std::size_t row) const noexcept {return row<kRows&&samplePresent_[row];}
 bool clearSample(std::size_t row);
 bool previewSample(std::size_t row);
 bool previewSampleEdit(std::size_t row,const SampleEdit& edit,
                        std::uint32_t revision=UINT32_MAX);
 bool stopSampleEditPreview();
 bool sampleEditorSnapshot(std::size_t row,SampleEdit& edit,std::uint32_t& frames,
                           std::array<WaveformRange,256>& envelope,
                           std::uint32_t first=0,std::uint32_t last=0,
                           std::uint32_t* revision=nullptr,
                           std::array<WaveformRange,256>* rightEnvelope=nullptr,
                           bool* stereo=nullptr);
 std::uint32_t editorWaveSampleRate() const noexcept {return editorWaveCache_.sampleRate;}
 bool commitSampleEdit(std::size_t row,const SampleEdit& edit,
                       std::uint32_t revision=UINT32_MAX);
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
 bool setStepVelocity(std::size_t pattern,std::size_t row,std::size_t step,std::uint8_t velocity,const void* owner=nullptr);
 void beginVelocityStroke(std::size_t p,const void* owner=nullptr);
 void endVelocityStroke(bool commit,const void* owner=nullptr);
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
 bool processorConfirmsPattern(unsigned requested);
 bool reservePreviewStop() noexcept;
 void invalidateTransientEditsForRecall() noexcept;
 // Reusable STOP command is reserved before any audition can start.
 Steinberg::Vst::IMessage* reservedPreviewStop_{nullptr};
 bool previewStopPending_{false};
 std::vector<DrumatorView*> views_;
 Steinberg::tresult sendEditMessage(Steinberg::Vst::IMessage* message);
 std::array<std::uint8_t,kCells> grid_{};
 std::array<std::uint8_t,kPatterns> patternLengths_{{32,32,32,32,32,32,32,32}};
 PatternSteps clipboard_{};
 std::uint8_t clipboardLength_{32};
 PatternUndo undo_{};
 PatternSteps strokeBefore_{};
 KitEditGeneration editGeneration_{};
 KitEditGeneration::Token strokeGeneration_{0};
 std::size_t strokePattern_{kPatterns};
 const void* strokeOwner_{nullptr};
 bool clipboardValid_{false};
 bool replacePattern(std::size_t p,const PatternSteps& data,bool remember=true,
                     std::uint8_t requestedLength=0);
 void rememberUndoStep(std::size_t p,std::size_t r,std::size_t s,std::uint8_t next);
 std::array<std::string,kRows> labels_{};
 std::array<bool,kRows> samplePresent_{};
 std::array<std::string,kRows> customNames_{};
 // Filesystem navigation is session-only. Embedded WAV data remains self-contained.
 std::array<std::string,kRows> samplePaths_{};
 // UI thread only. A single row cache bounds decoded-PCM residency even
 // when several View windows/16 sample rows exist. Processor revision is
 // always checked before reusing it after edit, import or project recall.
 DecodedWav editorWaveCache_{};
 WaveformExtremaIndex editorWaveIndexLeft_{},editorWaveIndexRight_{};
 std::size_t editorWaveRow_{kRows};
 std::uint32_t editorWaveRevision_{0};
 std::array<std::uint64_t,kRows> originIdentity_{};
 std::array<bool,kRows> originIdentityValid_{};
 // Last rejected per-row origin paths, held only on the controller thread.
 // Detects a new Controller-first recall returning to a previously known
 // project after an unrelated unpaired Component-only state.
 std::array<std::string,kRows> invalidatedSampleOrigins_{};
 bool pathsRestoredBeforeComponent_{false};
 bool componentStatePendingController_{false};
 bool dirtyPending_{false};
 // Last host pattern callback, not yet reconciled with Component-state recall.
 bool hostPatternPending_{false};
 Steinberg::Vst::ParamValue hostPatternValue_{0.};
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
