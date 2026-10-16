# M09 — compact row UI and actual packaged VST3 audio-host test

## Implemented candidate
- A new kit displays **4 of 16 rows** by default, with a + button to add one row up to 16.
- A minus button hides the **last** row only when it has no loaded sample and no active steps in **any of the 8 patterns**. A refusal retains every note, sound and slot. The engine retains 16 fixed instrument slots, MIDI note mapping and the 32-step scheduler.
- Compact editor description starts at 1100x300 logical pixels; VSTGUI's official requestResize API is invoked when rows change. Actual resizing requires host cooperation and remains to be tested on Studio Pro.
- New processor state schema DMR3 stores visible row count (1..16), selected pattern and all 4096 step velocities. Legacy DMR1/DMR2 restore **16 visible rows** to preserve the original old-project layout. Invalid states are rejected without changing live cells.
- Controller message `Drumator.VisibleRows` updates the processor's count, separately from step and sample messages.
- Every row is retained internally. "Hiding" a row is not deleting it; user cannot hide a populated last row from the GUI.
- Empty selected row still behaves exactly as M08 (silence). All prior audio paths remain unchanged.

## Independent Windows QA (confirmed for source commit and run below)
- Standalone seven CTests, including `drumator_visible_rows_tests` with bounds, last-step-in-another-pattern and loaded-row guards.
- **New VST3 audio-host smoke**: loads the packaged PE DLL; calls official Steinberg factory/controller/processor APIs, sets audio bus and sample rate, connects peer endpoints; sends PCM16, PCM24, FLOAT32 synthetic WAV assets as binary messages, exercises actual stopped-transport MIDI input and checks produced float buffers against ground-truth sample levels. Reject a corrupt WAV, verify the previous sound continues and unloaded row 2 remains silent.
- Keep Steinberg Validator, moduleinfotool, exact package-tree inspection, independent artifact download/ZIP integrity, and packaged editor geometry verification.
- Real realtime p95/p99 host pacing, ASIO interaction, arbitrary WAV collections, Studio Pro-internal drag/drop, extended large-file handling, sample project recall, final 125A skin and DAW GUI layout still require separate work.
- This host simulation does not simulate physical drag/drop to a host-owned window and cannot establish compatibility of proprietary Studio One browser items. It is a real packaged VST3 DSP test, **not** an end-to-end Studio Pro GUI test.

## Resource conservation
Bundled source+QA changes into one Windows run, do not trigger builds for documentation-only commits. Preserve failure logs and correct real defects without relaxing assertions.

## Verification evidence (2026-10-16)
**M09 verified Windows QA:** source `c7e8a08eac6ab8fd2392d23151d44d18c2b81586`, [Actions #38956743559](https://github.com/challanger2000/125A-Drumator/actions/runs/38956743559) SUCCESS. Seven CTests **7/7 PASS**, official Steinberg Validator **47/47 PASS**, packaged editor geometry **1100x300 PASS**, actual packaged VST3 host simulation PASS for silence before load, PCM16 +0.50/+0.50, PCM24 +0.25/+0.25 and FLOAT32 -0.50/-0.50 L/R amplitude; corrupt WAV rejection preserves prior sound; unloaded row 2 silent. Artifact `125A-Drumator-M09-4-Rows-Host-Audio-QA` id `12070159256`, downloaded as ZIP, verified exact `125A Drumator.vst3` tree (3 nonempty files), valid JSON resources, PE x64 signature and ZIP integrity. NO claim of real Studio Pro browser drag/drop, GUI plus/minus interaction, recalled sample assets, prolonged paced realtime load or V1 final.

First Windows run [#38956225584](https://github.com/challanger2000/125A-Drumator/actions/runs/38956225584) compiled and passed 7/7 CTests, Steinberg Validator 47/47 and editor 1100x300, but the new packaged-audio host test **correctly failed** because `IAudioProcessor::setProcessing(true)` inherited SDK `AudioEffect::setProcessing` returning `kNotImplemented`. Explicitly implementing that lifecycle in source `c7e8a08` allowed the follow-up host smoke to verify actual PCM audio outputs. Both runs remain preserved.
