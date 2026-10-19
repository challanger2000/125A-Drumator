# 125A Drumator — 32-step graphical velocity lane (2026-10-19)

**Verified source:** `3944ed29875817f4ba2bf9058f95cf0332ce1cea`  
**Windows QA:** [Actions #39314206625](https://github.com/challanger2000/125A-Drumator/actions/runs/39314206625) **SUCCESS**, 11/11 CTests including new `drumator_velocity_lane_tests`, Steinberg Validator 47/47, packaged editor geometry and audio-host smoke PASS.

## Operation

- Click **VELOCITY-BALKEN OEFFNEN** below the step matrix to reveal a dedicated 32-bar velocity lane. Click **SCHLIESSEN** to restore the compact layout.
- Select the instrument row with the row < and > buttons. Clicking an ordinary sequencer step also selects that row and step.
- Click a velocity column to enter a value from **1 to 127**; hold the left mouse button and drag horizontally/vertically to paint values across steps.
- **Ctrl + left-click / Ctrl + drag** sets steps to **0/OFF**. No right-click or mouse wheel is required.
- The active column has an accent outline; bars 17–32 are visually muted while a 1T (16-step) pattern is selected. Their stored values are preserved and remain editable for later 2T use.
- All values are the same underlying `StepEdit` velocity values as the main sequencer, so the existing VST3 state/host DSP path handles playback and recall. No additional audio state serialization is introduced.

## GUI implementation

- Pure, deterministic `core/VelocityLane.h` centralizes exact 32-column hit geometry, 0/1–127 mapping and heights.
- The expandable lane requests more host window height and reverts its expanded state if the host refuses the resize. It does not overlay the 32-step grid.
- CView's existing legacy mouse handlers are extended with moved/up/cancel handling; painting is controller-thread UI work.
- `tests/VelocityLaneTests.cpp` uses always-active assertions so MSVC Release with `/W4 /WX` actually executes tests.

## Limits

- The Windows tests prove the full plug-in compiles and existing packaged editor/audio tests still pass, plus lane hit/value geometry. They do **not** automate an interactive mouse-drag within Studio One.
- Real mouse capture, Ctrl+drag, resize at 100%/150%, row selection, project reload and visible step-value correspondence in Studio One require owner testing.
- The lane is a functional VSTGUI prototype, not the final 125A-Branded vintage-studio GUI. **Not FINAL V1.**
