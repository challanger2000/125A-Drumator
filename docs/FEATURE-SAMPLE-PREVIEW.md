# Sample preview — 125A Drumator (2026-10-19)

## User operation

The per-instrument **P** button previews the WAV or FLAC sample already loaded in that row without pressing Play, provided the VST3 host continues delivering audio callbacks. The button is to the left of the instrument display name; regular name clicks retain sample loading and Shift+name retains independent row renaming.

Preview is a **one-shot**, not a transport or pattern start. It goes through the same sample playback, pitch, volume, pan, output bus, choke, mute/solo filters and master gain as an ordinary row trigger. If a row is muted or excluded by Solo, it remains inaudible. The preview action does not dirty the Studio One song or change saved pattern notes.

## Engineering implementation

- Controller sends transient `Drumator.PreviewRow` through `IConnectionPoint` without invoking edit/dirty tracking.
- Processor validates row, visible row limit, and whether a source sample is loaded on the UI/control thread. Empty and invalid rows fail instead of falsely reporting playback.
- An atomic 16-bit-use mask (stored as `std::atomic<uint32_t>`) queues one preview per row until the next non-empty process callback. Repeated button presses before that callback coalesce.
- Audio thread consumes that mask via one atomic exchange, translates each selected row to its existing `36+row` one-shot trigger, and uses the existing voice engine. The audio callback does not allocate, lock, read files or modify saved DSP parameters.
- Queued previews are cleared on sample replacement/removal, kit restore, processor deactivation and processing stop.
- Host limitation: if Studio One stops calling VST3 `process()` while stopped, the plug-in cannot emit preview sound until the host resumes processing. This must be checked in the real DAW.

## Evidence and release gate

Source commit [`959ce18`](https://github.com/challanger2000/125A-Drumator/commit/959ce183ba8b55637c66ac8d77f076eba5e44a22), Windows Actions [#39310135488](https://github.com/challanger2000/125A-Drumator/actions/runs/39310135488) **SUCCESS**. 10/10 CTests PASS, official Steinberg Validator 47/47 PASS, packaged editor smoke and packaged VST3 audio-host smoke PASS. New audio-host checks establish stopped-transport rendering, invalid and unloaded row rejection, non-retriggering and click coalescing.

Native Studio One GUI button response, paused-transport processing policy, external audio routing and ordinary project reopen still require user acceptance. **Not final V1**.
