# Milestone 03 — first SDK-backed VST3 test instrument

**Status: Windows x64 CI build and Steinberg bundle Validator VERIFIED on 2026-10-16; not a consumer-ready Drumator.**

Steinberg VST3 SDK 3.8.1, tag `v3.8.1_build_84`, root commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` and exact recursive tagged submodules.

Windows x64 VST3 *Instrument|Drum*, separated processor and controller, one stereo MAIN bus and one MIDI event input. Existing 32-step scheduler triggers the tested bandlimited PCM voice engine. A short **temporary internally synthesized test kick** is prepared in non-audio `setupProcessing` and plays on a fixed 4-on-the-floor placeholder pattern. This verifies host-to-scheduler-to-audio wiring without loading audio files or performing filesystem I/O in process.

Automatable Pattern parameter (stable ID 1000; eight discrete values) and two-word versioned state storing selected pattern. Sample-perfect Pattern automation, sample file loading, real editable 16-row drum kit, GUI, MIDI audition, multi-outs, missing-file restoration, reliable realtime asset handoff, project kit saving, and vintage DSP **are not yet implemented**. This milestone intentionally does **not** call an internal test tone a drum sampler.

When DAW tempo/PPQ validity flags are absent, no new steps trigger. On host STOP, voices stop and buffers become silent. No invented fallback beat clock. Invalid audio setup is rejected. Current 32-bit float processing is deliberate; 64-bit processing remains unimplemented.

Windows CI compiles the SDK instrument and existing standalone tests; the integration candidate is then packaged as a standard Windows VST3 bundle using the prescribed `Contents/x86_64-win` and `Contents/Resources` structure. The official Steinberg moduleinfotool generates and validates `moduleinfo.json`; the official Steinberg Validator tests the assembled **bundle**, not merely the binary. 125A Plugin Tester, actual functional audio integration tests, editor lifecycle and Fender Studio Pro 8.1.2 tests remain future release gates.

**No PASS claims without inspecting actual Actions evidence.**

## SDK packaging mode for this early no-editor integration candidate
The SDK builds a single-file Windows x64 `.vst3` binary with `SMTG_CREATE_BUNDLE_FOR_WINDOWS=OFF`, `SMTG_RUN_VST_VALIDATOR=OFF`, `SMTG_CREATE_MODULE_INFO=OFF`, `SMTG_CREATE_PLUGIN_LINK=OFF`. **These options do not disable validation**: the CI script assembles the standard Windows VST3 bundle, generates and verifies `moduleinfo.json` with the official moduleinfotool, and invokes the official SDK Validator on the assembled folder. Failures abort CI. The 125A-branded icon, final artwork, GUI resource handling, and complete delivery packaging remain separate requirements.

## Verified Windows milestone results (2026-10-16)
- Source commit: `6bf1835693e4271a2814086116924c6d08de2c59`.
- Actions run: https://github.com/challanger2000/125A-Drumator/actions/runs/38913822453 — **SUCCESS**.
- MSVC Windows x64 build: **PASS**.
- Standalone tests `drumator_core_tests`, `drumator_voice_tests`: **2/2 PASS**.
- Steinberg moduleinfotool creation + validation for `moduleinfo.json`: **PASS**.
- Steinberg Validator on packaged bundle: **47 tests passed, 0 failed**.
- Engineering-only GitHub artifact: `Drumator-M03-VST3-UNTESTED` (artifact ID `12052210407`). Not manually tested in Fender Studio Pro.
- Previous CI runs exposing SDK macro mistakes, invalid sample-rate assumptions and improper bundle validation are retained as failure evidence, not marked PASS.
