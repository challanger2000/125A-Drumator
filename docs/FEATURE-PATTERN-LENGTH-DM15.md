# 125A Drumator — independent 1T / 2T patterns (DM15)

**Source:** `1d6f5430bc376695f6bf4168eeece5e604ea8977`  
**Windows evidence:** [Actions #39312446384](https://github.com/challanger2000/125A-Drumator/actions/runs/39312446384) SUCCESS (10/10 CTests, 47/47 Steinberg Validator, packaged VST3 editor and real audio host smoke PASS).

## User control and behavior

- Pattern buttons show `A 1T` or `A 2T` (and likewise B–H).
- Ordinary left-click selects the pattern.
- **Shift + left-click** a pattern button toggles its own length between **16 steps (one 4/4 bar)** and **32 steps (two 4/4 bars)** without selecting another pattern.
- Step data 17–32 are retained, not deleted, while the pattern uses the shorter loop. Toggling back to 2T restores access to those steps.
- Within SONG mode, each chain entry still independently selects 8/16/32 total steps; when a 1T pattern occupies a longer entry, its first 16 steps repeat until that chain entry completes.

## Implementation and state

- No new heap allocation, file I/O, locking or sample preparation in the audio callback: each pattern's length is read from a bounded array of atomics at a block boundary.
- Transport discontinuity/start-at-bar-2 resets the pattern phase as before. Per-pattern length alters only the modulo used to select steps, not host PPQ tempo, MIDI event offsets, swing or sample playback.
- `Drumator.PatternLength` accepts only pattern indices 0–7 and lengths 16/32. Controller updates its visual state after a successful processor message. This is a project-state edit, not a host-parameter automation change.
- Versioned, checksummed **DM15** state appends eight length bytes after DM14 name metadata. DM14 and earlier project states default all patterns to 32 steps. Decode rejects invalid length values and preserves existing state on failure.

## Tests and limitations

- CTest verifies DM15 round-trip, invalid length rejection and DM14 backward-compatible project migration.
- Packaged VST3 host test measures the one-bar loop at 48 kHz/120 BPM with a continuous transport clock and ensures the two-bar alternative does not restart early. It also verifies processor message boundaries and serialized length recall.
- The first test in Actions #39311910830 failed because its fixture unintentionally inherited **Swing 35% and Humanize 70%** from a preceding test, so the assumed exact downbeat was no longer correct. Commit `1d6f543` isolates swing/humanize at 0 for this timing test and restores the previous values after it. The resulting Actions #39312446384 is SUCCESS.
- **Still pending:** manual interaction in Studio One 8.1.2, real project reopen, offline/online render comparison and final GUI acceptance. This is development, **not FINAL V1**.
