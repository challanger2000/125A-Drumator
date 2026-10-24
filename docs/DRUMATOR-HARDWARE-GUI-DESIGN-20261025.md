# 125A Drumator — Hardware GUI design direction, 2026-10-25

**Owner visual direction:** A photographed, premium studio drum machine that remains as quick and straightforward to use as the working instrument.

## Core visual contract
- Retain blue 125A identity, but keep steel subdued, desaturated and dark.
- Separate the 4 visible rows into graphite trays with true inset function and step wells.
- Raised button face, a small drop shadow and visibly lower selected state.
- Preserve the existing approved 101-frame knob filmstrip; add a restrained physical mount behind it.
- Pattern and row selection: restrained cool blue. Amber reserved for actual active step states; other status lights should be functional.
- 32 physical sequencer steps, exact existing clickable geometry, no imaginary bars, controls or displays.
- Actual WAV/FLAC peak miniature displays may be implemented later **only from real cached PCM extrema**, never faked.

## Functional and test contract
- No changes to audio engine, MIDI, controller parameters, host state, sample recall or automation.
- Visual-only changes in DrumatorView.cpp. Keep existing row sizing, tested scroll rail and mouse rectangles.
- Native VSTGUI renders at 100%, 120%, 130%, 150% and the 16-row scrollbar case must be inspected.
- 23 native test groups and Steinberg 47/47 required for Windows candidate; not proof of a Studio One PASS.
- All user-visible controls and editor states must remain legible, not obscured by decorative modules.
- Current candidate remains NOT FINAL until owner Studio One review/approval.

## Sequence
1. Gehäuse and overall hierarchy
2. Hardware button/knob material treatment
3. Sequencer and instrument tray styling
4. Cached waveform miniature from real PCM source (separate functional change)
5. Consistent sample editor and real host review
