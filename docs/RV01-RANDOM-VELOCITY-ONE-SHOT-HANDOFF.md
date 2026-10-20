# RV01 – One-shot RANDOM VEL. handoff to the new VSTGUI

Owner Studio One findings 20.10.2026: the old `RANDOM VEL.` knob changes only playback gain/velocity offsets; inline Velocity bars and saved steps remain unchanged. **This is wrong for the requested UX.** The desired control is an **action button, not a continuous percent knob**. No user-facing button should imply a value parameter.

## Controller implementation for visual GUI chat

New public method:
`Controller::randomizeRowVelocity(std::size_t row)`

- Trigger **once per left-click** on the instrument's `RANDOM VEL` button, not on mouse-down and mouse-drag combined.
- Applies only to the currently selected pattern, specified instrument row, and **steps within the active 1T/2T pattern length**.
- Only velocities 1..127 are modified; zero steps remain 0. No new notes, no mute/solo/pitch/output or other row affected.
- Bounded per-click ±24 MIDI-velocity relative variation; saturated values 1 and 127 handled so a press visibly changes their Velocity bar.
- A successful edit emits one existing `Drumator.PatternReplace` message and creates one Undo snapshot. The controller notifies views so all visible expanded Velocity lanes update immediately. Empty-row click is rejected without an Undo operation.
- Back-end Windows integration test `RANDOM one-shot existing notes` covers occupancy, second-bar preservation when 1T, active second bar in 2T, other row/pattern isolation, entire Undo restoration, and empty-row behavior.
- **Visual widget:** remove the 4th knob `RANDOM VEL. 0%` from the old row layout. Place a visibly actionable row button `RANDOM VEL.` in its old functional region. It must invoke the new Controller method for the row clicked, not the global existing `varyPatternVelocity()` (which applies across all rows).
- The legacy saved per-row `randomVelocity_` setting and `Drumator.RowRandomVelocity` message currently remain untouched so existing project state is still loadable. **Open migration decision:** when the new GUI removes the knob, old projects with nonzero saved realtime random velocity may still sound randomized, with no visible knob. Before final shipping, choose an explicit project migration or an accessible 'realtime' mode, or deliberately deprecate the effect with a regression test and versioned compatibility note. Do not silently hide active realtime behavior or break old projects.

## Parallel work / merge

The functional fix is developed in `fix/random-velocity-one-shot-2026-10-20` (based on the multiout fix). The graphical UI is being created by another chat: that work must not be overwritten. Merge the controller API and regression first; then the GUI chat must map the button to `randomizeRowVelocity(row)` and verify it directly in Studio One. This source addition **does not yet claim the button is rendered or the old knob removed**. Studio One human QA remains pending.

## Other owner items pending in audit

- SO01 additional VST3 output buses declared `kAux`: isolated `kMain` remedy and host tests, pending actual Studio One confirmation.
- Display current MIDI Velocity number 1..127 for clicked/dragged inline bars, without cluttering 32-step grid.
- Clarify global **Pattern length** versus each independent Song-entry length.
- 4-bar concepts must not silently expand `kSteps=32` and break DM15/DMR4 state. 8-bar chains should be handled via chaining. Design approval and migration/testing needed before changing core sequencing.
- Do not label a GUI release approved until independent owner DAW tests.
