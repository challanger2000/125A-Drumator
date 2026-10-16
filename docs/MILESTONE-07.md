# Milestone 07 — first VSTGUI step-grid prototype

Status: WINDOWS CI BUILD/TEST/VALIDATOR VERIFIED for the M07 source, but independent host editor lifecycle and user-file import still UNTESTED.

## Design
- 1100x600 VSTGUI window; buttons 100% / 150% (1650x900). Confirm usable at 1920x1080 host resolution.
- 16 rows with WAV-select region, 32 sixteenth-note step buttons, eight pattern buttons A-H.
- Click step toggles OFF -> velocity 100 -> OFF. Dedicated velocity lane is not implemented.
- Row click invokes native VSTGUI file chooser. PCM16 WAV mono/stereo only, message <=16 MiB.
- Filesystem read and WAV decoding on UI/control thread, not real-time audio thread; final loader needs a worker.
- Component-to-processor messages carry StepEdit or LoadPcm16Wav; GUI only confirms sendMessage success.
- Audio thread reads individually atomic step cells; first temporary factory kick still exists.
- Distinct state schema DMR2: 8-byte header plus 4096 step velocities; legacy DMR1 recognized.
- Generic VSTGUI design, NOT final vintage UI. Official 125A logo geometry remains authoritative from 125A-Branding.

## Known unimplemented and not release-ready
- Sample filenames/contents and row settings not yet saved with project state; project recall loses loaded audio.
- No running light/playhead indicator yet; only interactive steps.
- No file drag/drop, FLAC/AIFF, live velocity bar editor, multi-outs or finished branding.
- Host connected-controller messaging, audio output with imported WAV, editor reopen and file picker remain unverified until actual Fender Studio Pro test.
- GUI sample load uses UI-thread I/O for this deliberately size-limited prototype, not final experience.
- Host getState() takes individually atomic snapshots, not a concurrent multi-edit transaction.
- Steinberg Validator success is not proof of end-to-end audio/UI behavior.

## Required validation
- SDK pinned Steinberg 3.8.1 including recursive tagged submodules.
- Six CTest suites including new atomic grid + existing scheduler/voices/WAV/exchange/MIDI.
- VSTGUI resource Drumator.uidesc inside bundled Contents/Resources.
- Official moduleinfotool, Steinberg Validator and eventual actual editor/UI-to-audio QA.
- No claims of finished Drumator or working DAW editor before Windows/host evidence.

## Confirmed Windows QA (2026-10-16)
Windows CI VERIFIED at source commit d1055c1b59aba15b6a7c9d594e5f882b13fd630d, Actions #38923496070: Windows x64 MSVC build PASS, CTest 6/6 PASS, official Steinberg Validator 47/47 PASS on assembled standard Windows VST3 bundle, moduleinfotool PASS; uploaded engineering-only artifact Drumator-M07-GUI-PROTOTYPE-UNTESTED (ID 12054908672). No actual Fender Studio Pro editor/interactive WAV import test, 125A Plugin Tester, sample file project recall or FINAL release.

**Scope:** No green CI result establishes a usable GUI window, GUI-file picker interaction, persisted sample paths, or user DAW sound playback until those are explicitly tested in Studio Pro. The first C++ const qualifier defect (run #38921864891), Windows missing official SDK moduleHandle (run #38922245302), and SDK helper macro source-root / path normalization issues (runs #38922848467, #38923220766) remain preserved in Actions history.
