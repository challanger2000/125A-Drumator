# 125A Drumator WAV editor — current operation and acceptance

The instrument row now offers a direct **EDIT** button. A loaded embedded sample is required. The modal offers START/END, draggable FADE IN/OUT, GAIN (0–400%, unity at 100%), ZOOM IN/OUT, FIT, scroll/pan, PREVIEW, STOP, RESET, CANCEL and APPLY. AUTO START and the earlier sample-menu WAV EDITOR entry were removed.

Edits are non-destructive. Imported FLAC is converted into embedded WAV. A full imported drumloop does not automatically synchronize with host BPM and is not time-stretched.

## Studio One owner acceptance (not covered by CI)

1. Load a WAV or FLAC, use row EDIT, confirm disabled EDIT on empty rows. Compare actual waveform and hit areas, including top/bottom/left/right edges, across 100/120/130/150% zoom.
2. Adjust START/END, both fades and GAIN (0/100/400%). Check PREVIEW/STOP and playback with transport stopped/running.
3. CANCEL discards the draft; APPLY commits it; save/reopen project and confirm identical sound and edit positions.
4. Exercise FIT, ZOOM, scrolling, Reset and visible errors after a separate view deletes/replaces the source.
5. Test one-frame/two-frame selection, full fade, silent gain, mono/stereo and project sample rates 44.1/48 kHz.
6. Test sample drops while modal is open, reopened editors, MIDI, Song, multi-output and ASIO/transport-loop transitions. Record build commit, host version, case and PASS/FAIL.

This documentation update does not establish real Studio One acceptance.