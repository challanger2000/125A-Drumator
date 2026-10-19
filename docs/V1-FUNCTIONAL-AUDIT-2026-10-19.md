# 125A Drumator — V1 functional audit (2026-10-19)

**Source baseline:** `develop/v1.0.0` at `08ec0e282125e3be50950fac68ba23932ca44a60`; the DM15 1T/2T implementation has verified source `1d6f5430bc376695f6bf4168eeece5e604ea8977` and successful Windows Actions #39312446384. The V1-SCOPE document is a DRAFT concept, not a product release authorization. See `AGENTS.md` and 125A Engineering START-HERE.

Interpretation: **Implemented** means a concrete UI/controller/processor path exists. **Host QA** means a Windows-hosted package test exists and has passed at an explicitly identified source commit. **Owner QA outstanding** cannot be replaced by Steinberg Validator or GUI geometry tests.

| Feature | Source status | Verification / remaining gate |
|---|---|---|
| Windows x64 VST3 instrument | Implemented | Windows Actions #39310135488 SUCCESS, Validator 47/47; not a final release |
| WAV PCM/float + FLAC import; row-specific file picker/drag-and-drop | Implemented | Decoder, packaged WAV/FLAC host tests; native Studio One browser drag/drop must be user-checked |
| 4 rows expandable to 16, up to 32 steps per pattern, A-H | Implemented | Packaged row/pattern tests; 16 visible at once depends on host resize/compact viewport |
| Independent instrument names | Implemented DM14 | #39307595099 SUCCESS; actual in-host Shift+label text entry remains owner QA |
| Per-row volume, pan, pitch, mute, solo, four choke groups | Implemented | Host control/state and voice-engine QA; user-reported +100 pan interaction needs owner regression |
| 16 stereo output buses including MAIN and selectable per-row destinations | Implemented | Packaged multiple-stem host test PASS; actual bus activation/routing visibility in Studio One needs owner QA |
| Step velocity: per-step data, step color, Shift+step cycling and +/- adjustments | Implemented basic | Packaged step edit and project recall; **dedicated graphical 32-step velocity lane/editor still not implemented** |
| Swing + deterministic humanize and per-row velocity randomization | Implemented | Deterministic split-buffer core QA; user-reported swing drift needs real DAW test |
| Sample retrigger, voice cap, choke fades | Implemented | 64-voice engine and choke QA; extended paced realtime soak not run |
| FILL 2/4/8/ALL/CLEAR, Snare 2+4, hat offbeat, gallop, ghost | Implemented | Core fill tests and GUI/menu handlers; owner mouse interaction gate |
| Copy, Paste, Clear, Undo and velocity variation | Implemented | Undo 10/10 core QA #39305359911; GUI interaction still owner QA |
| Song/chain A-H with selectable half/one/two-bar entries | Implemented | Packaged song A-B-A host tests; Studio One song automation and project reload separate |
| MIDI A-H pattern switching; DAW automation event ordering | Implemented | #39301479372 SUCCESS including packaged MIDI/automation ordering |
| DAW transport/PPQ, stopped MIDI audition, loop resets/start at bar 2 | Implemented | Host/core event tests; owner start-at-bar-2, seek/loop, swing/tempo and offline render tests outstanding |
| Standalone A-H pattern length 1 or 2 bars | **Implemented DM15; Windows QA PASS** | Shift+pattern toggles length; 16-step replay without erasing steps 17–32. Packaged host QA #39312446384 PASS; native Studio One QA pending |
| One-click per-row sample preview (P) | Implemented | #39310135488 SUCCESS packaged stopped-transport audio test; host callback behavior while stopped remains DAW gate |
| Portable embedded kits and state backward compatibility | Implemented DM14, DM15 candidate | 64 MiB kit and all-16-row host recall, invalid-state rollback; DM15 must preserve DM14 and predecessors |
| Vintage sampler color with independent bypass | **Not implemented** | Optional feature in draft scope; requires owner acceptance and measurable DSP plan before implementation |
| Original professional vintage-studio final GUI / official 125A master art | **Not implemented** | Prototype functional, branding master not yet integrated; GUI-specific owner QA outstanding |
| Native Studio One acceptance, tester and final V1 release | **Not verified** | Cannot label FULLY FUNCTIONAL/FINAL from CI alone |

## Priorities before final-branded VSTGUI

1. DM15 1T/2T functional pattern length is now Windows-verified; complete native Studio One interaction and project-reopen QA.
2. Implement a genuine visual velocity editor (32 bars per selected instrument) without interfering with FL-style step workflow.
3. Reproduce and close owner-reported swing timing and host position at bar 2, output activation/pan limit, and native sample preview/renaming interaction.
4. Scope and design final VSTGUI using official master branding; no new effects or speculative DSP without explicit scope acceptance.
5. Perform full Windows VST3 QA and Studio One acceptance, verify exact artifact provenance before release.

**No code-backed evidence that optional SP-1200-style coloration or a dedicated velocity-lane GUI already exists.** Do not reimplement confirmed working Fill, Multi-Out, Copy/Paste, names or preview.
