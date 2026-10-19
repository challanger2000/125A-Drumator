# Instrument names: DM14 / 2026-10-19

## Implementation and user action

- Every row has an optional independent user-defined display name, up to 64 UTF-8 bytes.
- In the current VSTGUI prototype, **Shift + left-click** a row's visible name, type in the field, and confirm with Enter or focus loss. A normal left-click still opens the WAV/FLAC picker.
- Removing/replacing a sample leaves the custom instrument name intact. An empty custom name falls back to the sample basename or the empty-row prompt.
- Source WAV basenames remain stored separately; custom names never replace or alter audio asset filenames.
- A labelled, empty row cannot be removed inadvertently by the minus-row control.

## State and compatibility

- New bounded, checksummed `DM14` kit state appends 16 independent label fields after the legacy sample data; this avoids reinterpretation of existing sample filenames.
- `DM13` and earlier states load without custom names and keep their original sample bytes, patterns, and routing. A corrupted DM14 label or payload is rejected without partially replacing the previous state.
- The VST3 `Drumator.RenameRow` control message validates index, length, and UTF-8; name changes occur outside the realtime audio callback.
- No automatic migration of an old user's files on disk; new state is written on the next DAW project save.

## Evidence

- Tested source commit: `c62b8faff07d27780edf10383a5e1283746913bb`.
- GitHub Actions [#39307595099](https://github.com/challanger2000/125A-Drumator/actions/runs/39307595099): SUCCESS; 10/10 CTests; Steinberg Validator 47/47; packaged editor geometry and audio host smoke PASS, including name messages and serialized-state validation.
- Interactive inline editing, actual keyboard/focus semantics in Studio One 8.1.2, UI zoom behavior, native project reopen and long-running realtime stress **not verified by this automation**.

## Remaining separate requirements

- Sample audition/preview via an explicit GUI action is not yet part of this milestone.
- Complete final-branded GUI and Studio One manual acceptance remain outstanding; no final V1 approval.
