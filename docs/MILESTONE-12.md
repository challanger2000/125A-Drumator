# Milestone 12 — Full kit recall regression and V1 completion gate

Status: DEVELOPMENT; no release approval. Baseline: M11 Windows Actions #38964241278 SUCCESS, source 2b628177534027c49d3590176891d9f64d7ed70d.

## Added M12 regression
The packaged VST3 host simulation now saves all 16 loaded FLOAT32 WAVs, creates a completely new processor, restores DMR4 before setupProcessing, initializes the output bus and audio lifecycle, and numerically verifies every row's stereo playback. This extends M11's serialized-byte test. It is *not* evidence of native Studio One state recall or an all-day real-time workload. M12 must not be labelled PASS until Windows CI for its exact commit finishes successfully.

## Implementation gap against docs/V1-SCOPE.md
Current GUI supports four visible rows by default, expandable to sixteen, eight pattern selectors, binary step toggles, WAV file selection/drop when OS path is supplied, and 100/150% scaling. Missing/not demonstrated as implemented:
- FLAC ingestion and independent instrument names
- Row volume, pan, pitch, mute, solo, and channel output assignment
- Per-step velocity *editing* and visual editor, swing and deterministic humanize
- User-controlled choke groups, preview, copy/paste, undo
- Multiple stereo output buses and routing verification
- Optional bypassable vintage sampler coloration
- Complete GUI styling and interaction testing in Studio One

The core tests may cover supporting internal behavior; that does not establish user-facing V1 operation.

## Completion rule
Do not call a build FINAL or V1.0.0 merely because CI is green. Each committed function requires action → controller/state → DSP/host effect → GUI feedback → save/reload, plus invalid-input and realtime safety checks. Native Studio Pro 8.1.2 project open/reopen, drag-drop and audio QA remain separate owner-tested gates. Preserve M11 as last verified milestone until the new one passes.
