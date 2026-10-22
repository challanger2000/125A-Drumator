# 125A Drumator — WAV editor / implementation contract

Status: core implementation started; not yet a functional VST3 editor.

The editor opens as a compact overlay inside Drumator per instrument (EDIT).
Controls: waveform, draggable sample-accurate START / END markers; visible fade-in and fade-out curves; AUTO START; RESET; waveform zoom; GAIN; audition; OK / CANCEL. No separate OS window. UI reference area about 700 x 450 before host zoom.

Edits are non-destructive: original WAV/FLAC decoded source is retained. Frame region is half-open [start,end); fade length and gain act on playback only. CANCEL must not mutate the sample, playback or project. OK must commit one coherent edit transaction.

Audio thread must not decode, allocate, lock, or destroy PCM. Integrate immutable prepared assets and deferred retirement into existing SampleExchange; active voices keep safe asset lifetime. Metadata must survive KitCodec save/reload and full DAW project recall, using a backwards-compatible NEW state version (not an unguarded change to DM15). Rejected or corrupt states leave old kit and edits unchanged.

QA gates: mono/stereo, 44.1/48/96k, start/end exactly at boundaries, zero fades, long fades and fade non-overlap, 1-frame trim, GAIN 0/100% semantics, repeated edits while playing, preview parity with committed sound, cancel/undo/reset, editor attach/detach, zoom, and full saved/recalled audio+metadata. Studio One practical owner acceptance is separate.

Current milestone: standalone, host-independent SampleEdit model and waveform-envelope geometry, with CTest. Controller, processor, kit serialization, popup drawing and audition are not yet connected. Do not report usable WAV editor or final QA until those are implemented and tested.
