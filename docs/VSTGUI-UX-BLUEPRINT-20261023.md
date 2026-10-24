# 125A Drumator – UX / VSTGUI design-system blueprint
Status: reviewed implementation plan, not a verified redesign or release.
Source baseline: `81dc757157793d1fb0c3a65e766c2232cbca0a70`, Windows QA run #39981766245.
Scope: user-facing design, interaction and maintainable VSTGUI; do not change the audio processor, MIDI map, sample serialization, controller state, or VST3 buses.

## Source audit and usability findings
1. Title drawn at `{16,5,304,35}` left aligned. Move `125A DRUMATOR` to centered header with a dedicated, reserved rectangle. Never overlap BAR toggles, zoom, pattern row or transport; align by actual view center x=640, not by text padding.
2. Bar selection buttons `1 TAKT` (66 px) and `2 TAKTE` (75 px) differ in width and language. Use one fixed button preset width 84 px, height 28 px; captions `1 BAR`, `2 BARS`. Both render and hit-test from the same declared rectangles.
3. `drawControl()` shares paint only. Most event handlers independently spell out their x/y ranges; this permits invisible and dead click areas after resizing (already observed with moved WAV buttons and stale integration coordinates). Replace rectangles and handlers with a shared `UiAction` table: unique ID, semantic action, role, rectangle, selected, enabled, tooltip. Render and dispatch from the same table.
4. Menu `PATTERN ...` currently draws German `KOPIEREN, EINFUEGEN, LOESCHEN, RUECKGAENGIG, VELO VARIIEREN, VELO 100, DUPLIZIEREN`. Adopt `COPY, PASTE, CLEAR, UNDO, RANDOM VELOCITY, RESET VELOCITY, DUPLICATE` without changing underlying action IDs/order.
5. Per-row SAMPLE menu mixes German actions. Add a direct `EDIT` button adjacent to sample name; it is disabled when no sample is present, and invokes existing `openSampleEditor(row)`. Remove duplicated WAV EDITOR menu entry only after the direct action is wired and tested. Sample name remains a visible click-to-load surface. Do not silently change Shift rename or Alt clear until usability migration is intentional and documented.
6. Current widths vary by role (26 px menu/prev/next, 36 px PLAY, 49 px FILL, 34 px M/S, 80 px OUT). Uniformity means *role-consistent* sizing: all icon buttons 32x30; all row action buttons 64x30; all toggles 32x30; all paired BAR buttons 84x28; menus 100%-consistent row height 28, padding 4. Not all controls must have identical width if labels and purposes differ. The implementation should use at most a handful of reusable sizes and one button component.
7. `UiCompactLayout.h` row = 94 (+30 expanded), `UiRowGeometry.h` row = 29 (+27). These are conflicting conceptions of row geometry; identify the actual active path before refactoring; retire legacy duplicates only after tests. Add geometry tests to enforce one coordinate system, including high zoom and short host window.
8. Row/sample/menu actions, Song editor, Fill popup, velocity bars, global rotary controls and WAV modal must use consistent focus/hover/pressed/disabled/selected styles. Button activation must remain left-click friendly; no right-click or mouse wheel required.
9. Preview on the sample row and Preview/Stop within WAV modal are *different scopes*. Label via proximity/group style; stop affects only WAV audition; no host transport confusion.
10. The main UI mixes `+ ZEILE`, `- ZEILE`, `2 TAKTE`, `1/2 TAKT`, `+ EINTRAG`, `LETZTEN LOESCHEN`, `ZOOM`, `LEER`, `STEPS 17-32: 2 TAKTE WAEHLEN`, etc. Translate all visible menus, popups, tooltips, errors and status strings into English, including temporary status for no sample/failed actions. Do not localize filenames or private firmware/source metadata.
11. Long labels may clip: measure text and use ellipsis inside fixed bounds; never widen a button according to translated label. Numbered sample rows, source filenames and MIDI labels must remain legible.
12. A modern GUI must keep sample editing quick: sample name > EDIT > START/END/FADE/GAIN > PREVIEW/STOP > APPLY/CANCEL. Separate frequent and destructive commands; remove duplicate entries.
13. Pattern lengths and Song entry lengths are not the same controls. Their labels and settings must not masquerade as each other's state; retain distinct state indicators.
14. Zoom states 100%, 120%, 130%, 150%: VSTGUI logical coordinates transformed uniformly with proper pointer hit testing; explicitly test 100% and 150% without losing modal controls or changing knob values by opening a menu.
15. Explicitly document one-shot focus: WAV/FLAC longer loops can be loaded, but there is no automatic BPM detection, DAW-tempo lock or time stretching. Do not advertise loop-sync.

## VSTGUI component vocabulary (reusable classes, not bespoke per-button code)
- `UiButton`: role (primary, secondary, toggle, destructive, icon), action ID, label, icon, state. One implementation; apply palette/geometry tokens.
- `UiRotary`: shared VOL/PAN/PITCH/MASTER/SWING/HUMANIZE behavior, text value, safe reset and drag sensitivity.
- `UiSampleSlot`: fixed filename display, load, EDIT, browse/previous/next, row audition and context menu. Sample-present enables EDIT.
- `UiStepCell`: 16/32 grid cells with active, disabled, accent and Velocity relationship; no one-off sizing by row.
- `UiVelocityLane`: a standard row-relative layout, clip controls within row.
- `UiPopup`: shared row/menu model with bounded scroll/position and fixed menu item height.
- `UiModal`: Sample editor and Song overlay use common framing, buttons and dismissal rules.
- `UiStatus`: error/progress only; no permanent instructions occupying waveform space.

Suggested design tokens (logical px, adjust only as whole families): `space=4/8/12`, `buttonH=30`, `iconButtonW=32`, `rowActionW=64`, `pairButtonW=84`, `popupItemH=28`, `cornerRadius=4`, `textSize=12`, `labelSize=11`. Avoid forced per-caption width changes. The fixed 1280 logical canvas remains until a separate responsive-layout audit.

## Acceptance gates before finalizing the new GUI
- One source of truth for painting and hit zones, a non-overlap/layout assertion for each supported zoom, test every action and popup.
- Direct EDIT loaded/empty behavior; disabled state and sample change while modal open.
- Stable DAW project recall, instrument count/paging, controls at 100/130/150% and Studio One window limits.
- Preserve existing 23/23 CTests, 47/47 Steinberg tests, add new GUI action/geometry and localization coverage; manually verify text width, feedback and modal rendering in Studio One.
- No new feature or release claim just because the GUI visually appears finished.
