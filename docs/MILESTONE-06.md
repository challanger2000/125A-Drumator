# Milestone 06 — VST3 WAV-message bridge and live MIDI audition

Status: **Windows x64 compilation, five standalone CTests and official VST3 Validator verified** for this source commit. The VST3 message receiver has NOT been exercised end-to-end with a GUI/DAW WAV import, so this is not a user-facing sample-loader milestone.

## New processor-side capability
- Steinberg **IConnectionPoint::notify** (SDK 3.8.1 UI-thread contract) accepts message ID `Drumator.LoadPcm16Wav` with binary attribute `WavBytes` and integer attribute `Row` (0–15).
- Rejects malformed/unsupported PCM16-WAV and message payloads larger than **16 MiB**. The latter is a temporary integration transport budget, *not* the intended maximum sample/file size of the finished instrument.
- Calls `PreparedSample::fromWav` only on the VST3 UI/control thread; all PCM decoding and sinc kernel preparation stay outside `process()`. Publishes via `SampleExchange`; previous pending publication is never overwritten silently.
- `process()` adopts fully prepared samples at block boundaries. It only exchanges pointers and performs bounded reclamation *enqueue*, never destroys PCM buffers. The non-audio `notify()` path drains the reclamation queue.
- Existing one-shot voices retain their original PCM/kernel pointers after sample changes until they stop.
- A host sample-rate reconfiguration resets voices and clears row playback; sample-specific kernels must be prepared again at the new rate. Already owned old asset memory is retained until a safe replacement/reclamation or shutdown. This is not yet the final project-recall or automatic reprepare strategy.

## MIDI audition
- VST3 NoteOn pitch 36–51 maps to Drumator row 1–16 (indices 0–15), `velocity` mapped to 1–127, NoteOff ignored for one-shots.
- Sample-offset sorting merges MIDI auditions with the running-light sequencer in a fixed-size event batch. The callback uses no heap allocation.
- MIDI audition is allowed with host transport stopped. Transition from PLAY to STOP resets previous sequencer tails exactly once, rather than wiping every subsequent MIDI audition block.
- Missing PPQ or tempo still suppresses new sequencer events. MIDI note auditions are independent of host position. Event capacity is bounded, with non-essential excess MIDI events dropped; telemetry/backpressure is still needed.

## What is NOT yet implemented
- There is **no VSTGUI editor, file dialog or drag/drop**. Consequently, the processor's binary message receiver is currently a tested architectural entry point **only if a sender is provided**, not a user-facing WAV import feature.
- Audio file references, per-row settings and pattern data are not yet saved in a complete VST3 project state; the existing controller stores just the single pattern selection.
- Sample loading is UI-thread-bound for this initial message protocol and can block that thread on large data. Final version must use an appropriate off-thread loading worker or streaming/decoding policy.
- This VST3 bridge is not yet independently exercised by a host application. Existing PCM exchange and MIDI batch regression suites are not proof that the host connects the controller to the processor, nor that all GUI/event lifecycles are correct.
- The current packaged binary remains an **engineering-only integration candidate** with a temporary synthesized kick on row 1 until a user-file-loading GUI is implemented.

## Verification plan
- Compile with pinned SDK `v3.8.1_build_84`, Windows x64 MSVC; strict tests `/W4 /WX`.
- CTest: old sequencing, voice, WAV, exchange plus new MIDI trigger batch (**5 suites** expected).
- Official moduleinfotool and Steinberg Validator on assembled VST3 bundle.
- Subsequent step: a host exercise that creates a controller connection, passes real WAV messages and checks actual audio rendering with MIDI and transport. Later: VSTGUI file picker/drop with state persistence and release QA.
- Do not label any file-import UX as functional before verified message-sending GUI and real host tests.

## Verified Windows evidence (2026-10-16)
**Verified M06 technical build:** source `9f223c0f6c4d8e65a8ce368afd08a0387643af57`, [GitHub Actions #38919501529](https://github.com/challanger2000/125A-Drumator/actions/runs/38919501529) **SUCCESS**: Windows MSVC x64 VST3 build PASS; five CTests **5/5 PASS**; moduleinfotool PASS; official Steinberg Validator **47/47 PASS** on assembled bundle. Engineering-only artifact `Drumator-M06-INTERIM-VST3-UNTESTED` (ID `12053822596`). No DAW WAV-import/GUI/audio test or final release.

This Windows run validates the provisional binary's compile/link and Steinberg host-contract checks. CTest does not yet inject a WAV message into the actual processor instance; claim only a source-level integration, not real in-host sample-file playback.
