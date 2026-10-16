# 125A Drumator — V1 concept and engineering scope

Status: **draft concept**, not a locked specification and not an implemented plugin.

## Product promise
**Load a drum sample, click steps, and play.** A direct FL Studio Channel-Rack-like drum programming workflow, but its own design and implementation. No sequencer monster.

## Agreed product direction
- Sample-based **VST3 instrument**, Windows x64 only.
- 16 concurrently visible drum/instrument rows; a **32-step** classic running-light sequencer (initial design target).
- Drag/drop of samples into rows; WAV and FLAC are mandatory design targets; AIFF and further common formats require verified decoder support and licensing.
- Editable **instrument name** independent of the filename. Preserve pattern/step data when replacing a sample.
- Per-instrument volume, pan, **pitch** (not step pitch), mute/solo, audio-output assignment.
- **Per-step velocity**, visual velocity editor, humanize (timing/velocity) and swing.
- One-shot sample playback with well-defined voice/retrigger limits; **choke groups** especially for open/closed hi-hats.
- Multi-output stereo routing with safe semantics and no duplicate MAIN signal; exact bus arrangements require VST3/host verification.
- DAW tempo/transport/loop synchronization, reliable offline rendering, project state recall and pattern editing.
- Sample preview, copy/paste and practical undo support.
- Optional independently bypassable **vintage sampler character**: CLEAN first, SP-1200 STYLE as the first proposed coloration. More modes only after proven benefit.
- Plain VSTGUI functional prototype first. Final GUI: *original* vintage 1980s studio-machine atmosphere combining E-mu Emulator I blue/blaugrau inspiration, Oberheim-style grouping and clear drum-machine step buttons. Follow current 125A-Branding master; no direct copied hardware graphics.

## Deferred deliberately
No AI beats, no automatic arranger, no sample-search AI, no complex per-step parameter matrix, no multisample velocity-layer system, no built-in compressor/EQ/reverb, no unrelated DSP effects, no advanced polyrhythm engine.

## Proposed defaults — require written acceptance criteria
- 32 steps at 1/16 note resolution; eight selectable patterns A–H are proposed, not yet final.
- Up to 16 stereo output buses total (including MAIN), pending VST3 host verification.
- Humanize 0% must be neutral; randomized timing must be deterministic for offline bounce, avoid out-of-bounds/early-event scheduling.
- Sample replacement must never erase user programming; file decoding and sample swapping must not block the realtime thread.
- Precise behavior for trigger overlap, voices, choke fade, missing files, output disabled in host, transport discontinuity, pattern switching, sample changes while sounding, and project-portability must be specified before coding.

## Required validation
For each control: action -> controller/state -> audio/host behavior -> GUI feedback -> automation where applicable -> project recall. Verify timing against exact host PPQ/sample positions, no audio-thread file I/O/locks/allocations; stress voice limits, tempo jumps, loops, sample format/samplerate, multi-out, sample replacement, silence and offline/realtime parity. Pass actual Steinberg Validator, 125A QA, and user test in Fender Studio Pro before any release claim.

**No DSP or GUI implementation is approved merely by this concept file.** Design, research, acceptance tests and implementation must respect `125A-Engineering/START-HERE.md`.
