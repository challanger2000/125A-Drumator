# 125A Drumator — WAV editor: implementation and acceptance

## Implemented on development branch

The sample menu contains **WAV EDITOR**; the in-plugin modal overlay displays the embedded WAV waveform, START and END markers, gain, fade-in/out buttons, zoom in/out, reset, threshold-based AUTO START, provisional audition, OK and CANCEL.

Edits are non-destructive: the original source bytes stay embedded. The project state uses DM16 and migrates earlier formats. START/END use half-open source-frame intervals. Fades do not overlap; gain is linear (100% = unity). Prepared sample assets are produced on the control thread and adopted by the realtime thread without decode, allocation or deletion. The separate audition voice is not stored in the kit. CANCEL and successful OK stop that provisional voice.

## Automated test scope

Windows CI includes sample-edit edge cases, state migration, integration of the provisional audition output under stopped and playing transport, cancel-state neutrality, OK, recall and repeated native editor attach/detach. A green CI run is **not** a Studio One practical acceptance result.

## Studio One owner acceptance — follow in this order

1. Install the resulting Windows VST3 and load Drumator as an instrument in Studio One.
2. Import a WAV kick, use its row's sample menu → WAV EDITOR. Check that its waveform and marker positions are visible at the actual editor zoom.
3. Move START and END, change gain and both fades; **VORHOEREN** must sound different from the untouched source. Try ZOOM +/−, RESET and AUTO START.
4. Select ABBRECHEN and trigger the original row from a note/step. The original sound must be preserved.
5. Open WAV EDITOR again; choose an audible edit, select OK and trigger the row. The committed edit must be audible.
6. Save the DAW project, close and reopen it. The WAV must play with the committed edits; reopen WAV EDITOR and check START, END, fades and gain.
7. While Studio One transport is playing, open editor and audition again; ensure patterns continue correctly, preview stops on CANCEL, and no crash or sustained sound remains.
8. Close and reopen the plugin editor repeatedly. Test at 100%, 130% and 150% GUI zoom where the host permits.
9. Test mono and stereo WAVs, at 44.1 kHz and 48 kHz project rates. Confirm silent 0% gain, one-frame trim and minimum/maximum boundaries cannot crash or corrupt the project.

Record PASS/FAIL and the exact failing action. Do not declare final until hands-on Studio One confirmation. Known UI limitation: waveform zoom does not provide scrolling/panning yet; AUTO START is threshold-based at the current waveform resolution, not a sample-perfect transient detector.
