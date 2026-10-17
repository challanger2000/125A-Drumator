# 125A Drumator — Independent Pre-GUI Audit (2026-10-17)
Scope: branch develop/v1.0.0, through M37. Read-only source review of VST3 processing, provisional VSTGUI, controller, resources and existing QA. Independent Codex second pass requested. **No real Studio One GUI rendering or manual host tests performed here.**

## Verdict
**ENGINE CI: PASS. PROVISIONAL GUI GEOMETRY: NOT PASS / further checks required. FINAL V1: NOT VERIFIED.**
Latest previous verified run: #39058731130 at commit 22567d6, SUCCESS. Validator/packaged audio smoke do not prove actual visual placement and clicks. No code changes or Actions required solely to record this audit.

## Confirmed GUI issues
1. **HIGH — Inconsistent view width (G01).** `src/vst3/DrumatorView.cpp::adjustEditorHeight()` calls `editor_.requestResize({1280*zoom,height*zoom})` and sizes its parent to 1280, but sets the **custom view** to `setViewSize({0,0,1100,height})`. Drawing uses x to 1280; row bands reach x1246 and zoom buttons x1217. Resource template in `resource/Drumator.uidesc` is 1280x300; `tests/EditorGeometrySmoke.cpp` tests just outer `getSize()==1280x300`. Potential clipping and wrong hitbox coverage. Fix logical widths consistently, verify attached host view.
2. **MEDIUM — Pattern click gaps activate hidden areas (G02).** Painted A-H buttons are 54 px wide every 62 px starting x340, y38..68. Handler takes full width62, including 8px visual gaps. Make hit area identical to drawn buttons.
3. **MEDIUM — Undiscoverable split hit areas (G03).** Volume x280..345, pan350..415 and pitch420..475 use label halves for -/+ without visible buttons. Swing185..244 and Humanize448..505 also split a generic "- / +" string. A bare choke group number at x535..558 has no descriptive header. Test every hit boundary and make actual actions visually discoverable.
4. **MEDIUM — Text and font metrics not validated (G04).** Fixed font 12px for row labels and values, no measured text bounds; filenames truncate by byte length (17 + ellipsis + final 7 bytes), unsafe for UTF-8/variable-width glyphs; some German copy is ASCII-only. Exact font centering/overlap and long labels at 100/150% **NOT VERIFIED**.
5. **MEDIUM — Responsive editor lifecycle not tested (G05).** Visible rows 1..16 change content height: max(300,108+29*rows+64), range 300..636. Check attached resize, parent/child view rect, clipping, bottom action/status bars, 150% zoom and Studio One window persistence. The geometry smoke doesn't attach/editor-draw.
6. **MEDIUM — GUI model-state synchronization (G06).** Local controller arrays store labels, grid, gain, pan, etc., while processor owns audio state. Serialized `setComponentState` covers recall, but full external automation/update, editor open-close-open, multiple instances and async consistency are unverified in real host. Perform action-to-audio-to-saved-state trace.
7. **MEDIUM — Drag and drop (G07).** `DrumatorDropTarget` accepts a single file path (WAV/FLAC) and specific row strip, not arbitrary proprietary DAW browser payloads. Actual Studio One Browser DnD is **NOT VERIFIED**. UI should distinguish handled drop from successful sample load.

## Provisional GUI control-to-coordinate checklist (logical px from source)
- Title x16..1000 y8..32; subtitle x16..320 y47..67.
- Pattern buttons A-H x340+62*n through x+54, y38..68. Mouse accepts x+62.
- Zoom 100% x1070..1127 and 150% x1136..1217 y39..69.
- Swing label x16..182; minus/plus x185..244 y74..98.
- Humanize label x275..447; minus/plus x448..505 y74..98.
- Instruction label x545..1240 y74..98.
- Row r top y108+29*r, row band x10..1246; number x16..47; name x52..275.
- Volume x280..345; pan x350..415; pitch x420..475; mute x478..500; solo x507..529; choke x535..558.
- 32 steps: x560+21*s, fill x+2..x+18; hit cell full width21; row y+4..y+22.
- Add row x16..213; hide last x226..266; row-count text x277..455; actions top y=108+29*visibleRows+5.
- Status text x16..1240 at y=108+29*visibleRows+39, height21.
All positions from static code. **Not pixel-verified screenshots.** Render/attach test 4 rows, 16 rows, 100%, 150%; exercise every control and boundary.

## Functionality inventory and evidence (CI evidence ≠ full DAW signoff)
- WAV PCM8/16/24/32, float, mono/stereo; FLAC decoder fixture: CI PASS.
- 16 instrument rows; 8 patterns x32 steps; 5 step-velocity levels: CI PASS.
- MIDI 36..51, invalid/zero velocity, sample offset, 64 voice limit, replacement and choke: CI PASS.
- Volume/pan/pitch/mute/solo, 16 stereo outs, auxiliary activation/fallback: CI PASS.
- Swing+humanize deterministic timing across blocks, transport/A-B-A sample-offset pattern automation: CI PASS.
- Embedded-kit project restore/fresh instance, malformed-state rollback; 48->44.1 kHz: CI PASS.
- Continuous generator tail `kInfiniteTail`: CI PASS.
- Actual Studio One DAW browser import, GUI redraw/hit targets, zoom/row resize, all 16 outputs, project reopen: **NOT VERIFIED**.
- Paced realtime deadline p95/p99/max, multi-instance stability, DAW automation edge cases: **NOT VERIFIED**.

## Additional code-level risks for Codex to evaluate
- `Processor::process` schedules each in-block pattern automation interval separately; verify multiple points, event offsets, equal-offset ordering, malformed host points, and trigger boundary inclusivity with swing/humanize. Existing synthetic test is not exhaustive.
- `setBusArrangements` accepts variable output bus count while full 16 stereo buses are registered. Verify true host negotiation and bus activation in Studio One.
- `getTailSamples` infinite is intentional for an internally scheduled generator; verify stopped-transport playback and host idle behavior.
- `setState`/sample exchange must remain transactional while audio processing; validate lock/ownership handover under rapid repeated load/unload and switching host sample rates.
- Current GUI resource allows max height650, while dynamic logical max636. Verify at host zoom 150% and the resizing contract.
- README retains historical milestone statements; do not interpret old release notes as current M37 readiness. Version source remains 0.1.0, **not FINAL**.

## Second-pass Codex instructions — READ ONLY
1. Independently inspect every source file and relevant tests on branch develop/v1.0.0; verify or reject G01–G07, cite exact file and line.
2. Audit all GUI controls, drawn vs clickable rectangles, fonts/text metrics and labels, alignment, clipping, scroll/resize, 100 and 150% scale, 1/4/16 row layouts. Explicitly mark things requiring actual screenshots/Studio One as NOT VERIFIED.
3. Trace every user action through controller/message/parameter/processor/DSP/state/reload feedback. Check boundary values and error handling.
4. Validate audio-thread safety, allocation-free process, deterministic timing, per-row choke/mute/solo/multi-out and sample ownership. Note performance aspects unmeasured in paced host.
5. Provide prioritized CONFIRMED / POTENTIAL / HOST-TEST-REQUIRED findings and proposed remediation/QA coverage; **DO NOT autonomously modify code or launch Actions**. Wait for owner review/approval.
