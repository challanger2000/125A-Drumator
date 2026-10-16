# Milestone 03 — first SDK-backed VST3 test instrument

**Status: source-only until a Windows build and tests prove otherwise. Not a consumer-ready Drumator.**

Steinberg VST3 SDK 3.8.1, tag `v3.8.1_build_84`, root commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` and exact recursive tagged submodules.

Windows x64 VST3 *Instrument|Drum*, separated processor and controller, one stereo MAIN bus and one MIDI event input. Existing 32-step scheduler triggers the tested bandlimited PCM voice engine. A short **temporary internally synthesized test kick** is prepared in non-audio `setupProcessing` and plays on a fixed 4-on-the-floor placeholder pattern. This verifies host-to-scheduler-to-audio wiring without loading audio files or performing filesystem I/O in process.

Automatable Pattern parameter (stable ID 1000; eight discrete values) and two-word versioned state storing selected pattern. Sample-perfect Pattern automation, sample file loading, real editable 16-row drum kit, GUI, MIDI audition, multi-outs, missing-file restoration, reliable realtime asset handoff, project kit saving, and vintage DSP **are not yet implemented**. This milestone intentionally does **not** call an internal test tone a drum sampler.

When DAW tempo/PPQ validity flags are absent, no new steps trigger. On host STOP, voices stop and buffers become silent. No invented fallback beat clock. Invalid audio setup is rejected. Current 32-bit float processing is deliberate; 64-bit processing remains unimplemented.

Windows CI compiles both existing standalone tests plus the SDK instrument and publishes an explicitly **UNTESTED** integration artifact. Actual Steinberg Validator, 125A QA, editor lifecycle and Fender Studio Pro 8.1.2 tests remain future release gates.

**No PASS claims without inspecting actual Actions evidence.**

## SDK packaging mode for this early no-editor integration candidate
The official SDK setting `SMTG_CREATE_BUNDLE_FOR_WINDOWS=OFF` builds a Windows x64 single-file `.vst3` module for this technical milestone. Steinberg explicitly supports this form; it does NOT stand in for final bundle-resource testing. The normal bundle path, 125A-Branding icon, metadata and resource lifecycle must be restored and validated before a user-ready Drumator release.
