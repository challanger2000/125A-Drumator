# M10 — portable embedded drum kit state (candidate)

**STATUS: M10 SOURCE CANDIDATE; Windows build, VST3 host end-to-end song recall and Steinberg Validator required before marking this verified.**

## Purpose

Previously M09 saved 4/16 visible rows and step velocities, but it **lost the actual loaded samples and row names** at every project reload. Store the exact original WAV bytes, without relying on user-specific absolute file paths or external missing sample files.

## DMR4 state format

- New distinct state magic `DMR4` (`0x34524d44`) and a bounded uint32 payload length.
- Payload: selected pattern ID (0–7), visible rows (1–16), all 4096 step velocities, then per-row length-prefixed UTF-8 basename and original RIFF/WAVE bytes. No absolute paths, no extra file-system access on recall.
- FNV-1a 32-bit integrity checksum over payload, plus exact expected size, per-sample WAV decoding and row range/step validity checks. Checksum is corruption detection, **not cryptographic authentication**.
- Maximum individual embedded WAV 16 MiB; total WAV bytes 64 MiB per kit. Above limit, reject the incoming WAV or state rather than produce a partially recalled kit. Saved project files may become much larger.
- Prior DMR1/DMR2/DMR3 project formats remain accepted and show 16 rows on legacy recall. Those older formats **never contained sample bytes** and cannot retroactively restore sounds absent from the saved project.
- Controller setComponentState reads the same DMR4 structure and restores row basename labels, visible count, all steps and selected pattern. No misleading "loaded" label if no sample is embedded.

## Audio threading

- VST3 controller loads a WAV from disk on UI thread once; sends validated original bytes and basename in `IMessage`.
- Processor prepares PCM and interpolation kernels OFF the audio callback; retains original WAV in an independent mutex-protected host/control-thread kit cache.
- `getState()` creates a bounded serialized snapshot from that control cache and atomic pattern grid.
- `setState()` fully parses/checks the replacement candidate before publishing; if processing rate is known, prepares every row before committing state. Loading while no output rate is configured keeps source WAV bytes for re-preparation during later `setupProcessing()`.
- Empty rows publish silence assets; previously present sample rows must not remain audible after a replacement kit omits their samples.
- The audio callback does not access the kit mutex, raw WAV cache, disk or decoder and never deallocates an asset.
- These are host/control-thread operations; a future work item is a dedicated worker for large WAV loading, memory profiling and strong concurrent project-save tests.

## Verification gates

- Standalone `drumator_kit_tests`: two embedded WAVs, filenames, grid/selected/visible count, deterministic roundtrip; checksum corruption, truncation, illegal basename and invalid RIFF rejection; failed decode leaves the prior candidate unchanged.
- Existing 7 CTests (now 8), actual VST3 editor geometry, 47 Steinberg validator checks.
- Actual packaged VST3 host smoke must save an original FLOAT32 sample to IBStream, create a **fresh component/controller**, restore state before setupProcessing, and verify the previously recorded waveform amplitude. This is a real instance roundtrip with no original WAV file available.
- Real Studio One project save/reopen remains a distinct host-specific acceptance gate. Audio/WAV loading, modern drag/drop, dynamic GUI resize, sample file limits, project save size and final branding remain unfinished or pending as applicable.
- Not FINAL, no release changes on `main`.

