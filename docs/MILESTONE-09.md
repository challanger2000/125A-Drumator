# M09 — compact row UI and actual packaged VST3 audio-host test

## Implemented candidate
- A new kit displays **4 of 16 rows** by default, with a + button to add one row up to 16.
- A minus button hides the **last** row only when it has no loaded sample and no active steps in **any of the 8 patterns**. A refusal retains every note, sound and slot. The engine retains 16 fixed instrument slots, MIDI note mapping and the 32-step scheduler.
- Compact editor description starts at 1100x300 logical pixels; VSTGUI's official requestResize API is invoked when rows change. Actual resizing requires host cooperation and remains to be tested on Studio Pro.
- New processor state schema DMR3 stores visible row count (1..16), selected pattern and all 4096 step velocities. Legacy DMR1/DMR2 restore **16 visible rows** to preserve the original old-project layout. Invalid states are rejected without changing live cells.
- Controller message `Drumator.VisibleRows` updates the processor's count, separately from step and sample messages.
- Every row is retained internally. "Hiding" a row is not deleting it; user cannot hide a populated last row from the GUI.
- Empty selected row still behaves exactly as M08 (silence). All prior audio paths remain unchanged.

## Independent Windows QA (pending until linked run PASS)
- Standalone seven CTests, including `drumator_visible_rows_tests` with bounds, last-step-in-another-pattern and loaded-row guards.
- **New VST3 audio-host smoke**: loads the packaged PE DLL; calls official Steinberg factory/controller/processor APIs, sets audio bus and sample rate, connects peer endpoints; sends PCM16, PCM24, FLOAT32 synthetic WAV assets as binary messages, exercises actual stopped-transport MIDI input and checks produced float buffers against ground-truth sample levels. Reject a corrupt WAV, verify the previous sound continues and unloaded row 2 remains silent.
- Keep Steinberg Validator, moduleinfotool, exact package-tree inspection, independent artifact download/ZIP integrity, and packaged editor geometry verification.
- Real realtime p95/p99 host pacing, ASIO interaction, arbitrary WAV collections, Studio Pro-internal drag/drop, extended large-file handling, sample project recall, final 125A skin and DAW GUI layout still require separate work.
- This host simulation does not simulate physical drag/drop to a host-owned window and cannot establish compatibility of proprietary Studio One browser items. It is a real packaged VST3 DSP test, **not** an end-to-end Studio Pro GUI test.

## Resource conservation
Bundled source+QA changes into one Windows run, do not trigger builds for documentation-only commits. Preserve failure logs and correct real defects without relaxing assertions.
