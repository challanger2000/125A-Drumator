# WAV-Editor Internal Audit — 2026-10-23

**Scope:** code-based review of editor input/drawing, non-destructive edits, audition, host lifecycle, storage and existing Windows CI. No claim of an actual Studio One test.

## Confirmed fixes in this audit
- Outputless processing now discards an outstanding WAV audition request and schedules a stop. This avoids a delayed audition when the host re-enables its output.
- START/END indicators and fade handles are clipped to the waveform viewing region while zoomed; offscreen marks no longer extend into neighboring controls.

## Findings still open / requiring specific coverage
- **Preview label:** The VORHOEREN/STOP button is a local toggle, not playback-completion feedback. After a short one-shot has ended naturally the label can continue displaying STOP; it does not indicate an actively sounding voice.
- **Fade visuals:** The editor overlays straight linear ramp lines; with extreme zoom or when a fade boundary lies outside the visible interval, truncated lines are display-only approximations. The DSP envelope remains in SampleEdit::amplitude.
- **Preview + master volume:** audition audio is mixed into Main before the master ramp, so its level follows the main master. Confirm this matches user expectations.
- **Zoom on tiny files:** scrolling/zoom at 1-frame source lengths is handled by source-frame geometry and should be tested with an actual WAV fixture.
- **Host rejected resize:** the editor refuses to open if it cannot expand the host view. This is safer than presenting inaccessible OK/CANCEL; a compact fallback is not implemented.
- **UI appearance/interaction:** pixel rendering and Studio One mouse tests, including 100/130/150% scaling, still need real host acceptance.

## Existing automated evidence
The preceding branch commits have green Windows CI, including the native VST3 Validator, SDK module test, integration audio playback, recall and native view exercises. The two changes above must pass a fresh CI before being marked PASS.

## Release gate
**NOT FINAL.** The audit documents unresolved usability behavior and missing host acceptance; passing the build alone is not a final release authorization.
