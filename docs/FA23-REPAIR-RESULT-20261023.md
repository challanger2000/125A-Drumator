# Drumator FA23 repair result — 2026-10-23

## Exact tested product

Commit: `9f1196ec75ec48cba657a96359b5003c458d1e83`
Branch: `fix/studio-one-acceptance-20261022`
Windows Actions: https://github.com/challanger2000/125A-Drumator/actions/runs/39943721567

The Windows x64 build and package validation completed successfully.

- **CTest: 23/23 PASS**
- **New Codex FA23 integration regressions: 10/10 PASS** (individually named in the workflow job log).
- **Official Steinberg Validator: 47/47 PASS**
- Packaged VST3 editor and audio host smoke tests: PASS

## Repairs mapped to independent findings

| Finding | Repair applied | Verification |
|---|---|---|
| FA01 BLOCKER | ClearSample atomically clears its SampleEdit metadata | FA23-01 PASS |
| FA02 HIGH | Per-row revision captured at WAV-editor open, checked for preview/commit, bumped by import/clear/edit/restore | FA23-05 PASS |
| FA03 HIGH | Consume preview-start request before sample-exchange adoption in each audio block | FA23-03 PASS |
| FA04 HIGH | Reset audition voices before starting a replacement one-shot | FA23-02 PASS |
| FA05 HIGH | Catch out-of-memory failures during binary WAV query inside VST3 notify | FA23-10 PASS |
| FA06 MEDIUM | Reject drag/drop target while WAV modal editor is open | FA23-04 PASS |
| FA07 MEDIUM | Distinguish explicit newly restored controller origins from orphaned previous component markers | FA23-06 PASS |
| FA08 MEDIUM | Finish active velocity gesture before taking immutable Undo tuple (pattern+length) | FA23-07 PASS |
| FA09 MEDIUM | Bound sinc interpolation to the selected audio frame domain | FA23-08 PASS |
| FA10 MEDIUM | Cancel active/pending audition during deactivation and stop processing | FA23-09 PASS |
| FA11 MEDIUM | Keep fade handles visibly drawn inside the waveform at the initial zero-fade boundaries | Code correction, practical pixel/host verification pending |
| FA12 MEDIUM | Limit large-amplitude waveform plot coordinates to the signal panel | Code correction, practical pixel/host verification pending |
| FA13 MEDIUM | Draw status/error text inside the active WAV modal above its background | Code correction, practical pixel/host verification pending |
| FA14 LOW | Replace obsolete implementation-state text in editor documentation | Source review |

These are fixes to the stated source-level defects, **not an assertion of full Studio One acceptance**. FA11–FA13 have no new dedicated native pixel regression in this batch, and source/DAW behavior cannot be inferred from compilation alone.

## Remaining release gates

- Native practical Studio One check with actual GUI mouse dragging and 100/130/150% zoom, plus host DPI variations.
- Complete user acceptance of the full instrument: WAV editor, Sequencer/Song, multiout, saved kits.
- High-pressure host/ASIO scenarios and actual mono/stereo/program-material signal checks where relevant.
- Product version is still 0.1.0; no V1 release declaration from these tests.

No intermediate GitHub Actions run was used between the first and final source-repair commits. This report-only commit does not match workflow source/test path filters and should not trigger Windows CI.
