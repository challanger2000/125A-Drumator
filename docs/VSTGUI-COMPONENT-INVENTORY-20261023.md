# Drumator GUI implementation inventory and hard gates
Source: reviewed development branch `fix/studio-one-acceptance-20261022` at `81dc757157793d1fb0c3a65e766c2232cbca0a70`.
This file is an implementation/QA inventory, not evidence that the new GUI exists.

## Visual and semantic hierarchy
**Top header:** authentic SVG master `125A_Logo_Master_FINAL.svg` from `125A-Branding` at left, the literal `125A DRUMATOR` visually centered at logical x=640, zoom at right. The title must not drift with layout changes, and no label or logo may intersect bar-selection controls. Design the header row height *before* moving controls; the current first row y=108 fixes the available budget.

**Global section:** pattern A–H selection, mode, 1 BAR/2 BARS pair, Song access, Swing, Humanize and Master. If the header cannot house the title and bar buttons without collision, rearrange the globals inside their existing 108 px budget or explicitly redesign the header size and propagate it through all row geometry; do not place title over live controls.

**Instrument row:** one consistent per-row template for number/selection, sample filename, EDIT, browser and preview, Fill, Mute/Solo, output, choke, Vol/Pan/Pitch, steps 1–32 and per-row Velocity lane. No bespoke row geometry for individual instruments. Audit alternate window heights and 4-rows-per-page.

**Song editor:** clear distinction between song state vs active pattern, chain slot, pattern A–H and 1/2/1/2 BAR-length options. Check every button/popup hits the correctly scoped state.

**WAV modal:** entire original editable source unchanged unless APPLY; separate preview transport, live fade overlay, trim markers, gain, zoom, scroll; preserve modal input capture and cancellation of drag/drop behind it.

## Fixed element families
| Family | Default logical geometry | Typical use | Rendering/action strategy |
|---|---|---|---|
| Small icon | 32 × 30 | Previous/Next, ellipsis, M/S if iconized | `UiButton(role=icon)` |
| Row action | 64 × 30 | EDIT, PLAY, FILL | `UiButton(role=secondary)` |
| Paired choice | 84 × 28 each | 1 BAR/2 BARS | `UiButton(role=toggle)` |
| Pattern choice | 49 × 30 each | A–H | `UiButton(role=toggle)` |
| Popup entry | consistent height 28, fixed popup width | Pattern/Sample/Output/Choke | `UiPopup` |
| Rotary | fixed diameter, label/value metric | Row Vol/Pan/Pitch + global | `UiRotary` |
| Step cell | current 19 px pitch, uniform state style | 32 steps | `UiStepCell` |

Families are design tokens, not permission to force all controls to have the same width. The required invariants are consistency *within a family*, fixed bounds across different text labels, and painting/hit testing of the same geometry.

## Verified source hazards requiring targeted regression
- `UiCompactLayout.h` declares row base 94/expanded +30, while `UiRowGeometry.h` defines a historical row height 29/expanded +27. Establish active callers before unification.
- `UiCompactLayout::sample` currently occupies x=78..272; menu 275..301; previous 303..329; next 332..358; preview 362..398; fill 403..452; M 457..491; S 496..530; OUT 535..615; steps start x=622. There is **no free 64px slot** for EDIT here without reorganizing the row. Adding EDIT by overlaying existing controls is prohibited.
- The four row knob centers are 133/254/375/496, and CHOKE has a separate line around x=570; use the full 94px-high row rather than only its 30px top strip when assigning EDIT, without obscuring knobs, choke, or step grid.
- `DrumatorView::draw` and `onMouseDown` manually repeat coordinates; any change must switch both to one geometry/action model before assessing UX.
- Pattern popup is German (`KOPIEREN`, `EINFUEGEN`, `LOESCHEN`, `RUECKGAENGIG`, `VELO VARIIEREN`, `DUPLIZIEREN`), with other German sample, song and error strings. Extract visible UI strings into one English catalogue and test completeness. Keep identifier and action order stable.
- The recent FA23 stale-modal test used literal click coordinates and previously failed after a toolbar relocation. Refactor UI tests to refer to action geometry and test disabled, hit and dismissal states to prevent this class of false regression again.
- The 125A branding master must be integrated from the true vector asset; no improvised logo text or recreation. Reusable control artwork should follow the approved `125A-Knob-Designer` assets; interim vector UI is not a substitute for the finished artwork.
- Sample EDIT enablement must be driven by the actual source presence, not the filename text (a renamed empty row can have a name).
- Status messages must explain genuine errors; normal UI must not dedicate large permanent regions to operating instructions.

## Required verification protocol
1. Before modifying source: confirm working branch and precise HEAD; check release provenance and code ownership. Keep docs/code changes separated by purpose.
2. Freeze the base geometry, component token catalogue, action ID inventory and English strings.
3. Introduce action/layout lookup without changing audio engine. Compare old/new hit map for all functional controls, including clicking empty space.
4. Add tests for no overlap, fixed geometry, disabled states, direct EDIT (sample present/absent), all popup actions, row paging, pattern/song state changes, Ctrl+click default reset, consistent English labels.
5. Static render at 100%, 120%, 130%, 150%, plus cropped host-window fallback and hover/pressed/disabled snapshots.
6. Compile Windows and run all existing 23 CTests + 47 Steinberg Validator tests; compare to previous PASS and repeat exact failed test before any new CI invocation.
7. Real Studio One session checks direct EDIT, mouse capture, saving/reloading kits, tooltips, and accessibility/readability. This last host gate is not substituted by green CI.
