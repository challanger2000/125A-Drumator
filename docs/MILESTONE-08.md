# Drumator M08 — truth in sample load, user WAV formats, and drop-to-row

## User-observed defect (2026-10-16)
Studio Pro 8.1.2 shows 16x32 grid after M07 resource fix. User hears something very quiet but bottom status still reports "WAV NICHT GELADEN"; drag/drop is absent.

**Root-cause distinction:** M07 still synthesized a very soft fixture kick on row 1 (0.35 generator amplitude × 0.8 row gain × -3 dB center pan). This made audio audible regardless of whether the user's WAV was really loaded. The load failure was **not verified to be a gain defect**. M07 only accepted PCM16 WAV even though common sample packs use PCM24 and IEEE float32. Other possible failures include message connection, payload size or audio setup; without the user's exact WAV and host status we must not assert which one happened.

## M08 source changes (to be verified in the next Windows build)
- Completely remove synthetic kick from process setup. Empty row plays nothing; default programmed Steps remain visible but are not mistaken for working WAV import.
- Decode strict RIFF/WAVE mono or stereo PCM unsigned 8, signed 16/24/32 and IEEE float32; WAVE_FORMAT_EXTENSIBLE subformats for supported PCM/float with matching valid-bits width. No FLAC, MP3, 64-bit float or RF64 claims.
- Decode validates chunk sizes, rate, alignment, frame count, nonfinite floats and unsupported/ambiguous format; transactional failure must not replace existing audio.
- WAV I/O and format preflight on UI thread (temporary <=16 MiB); decoder preparation on processor control thread, never in audio process. Not yet final worker loading.
- File selector or VSTGUI `IDataPackage::kFilePath` drop onto any of the 16 rows uses the same controller load method. Reject non-WAV, multiple-item, malformed or unsafe path payloads. Studio Pro may not send internal browser drag data as an OS file path; **actual drag/drop must be user tested**.
- No false success: label changes to real filename only after the processor `sendMessage` responds `kResultOk`. Status distinguishes no path, oversized file, unsupported WAV, disconnected controller/processor, and rejection. Message success means accepted for *next audio block*, not independently confirmed audible output.
- Fix transparent gain structure: row default 1.0 linear, 127/127 velocity => unity, center pan => unity per channel. No gain makeup, clipping, compression or fake peak normalization. Left/right balancing reaches zero on the fully opposite side. Existing pan tests must reflect this explicitly documented balance law.
- Step clicked ON velocity=127, OFF velocity=0. Legacy saved velocity values untouched.

## Tests and open release gates
- Extended decoder synthetic fixtures for PCM8, PCM24, PCM32, float32 and WAVE_FORMAT_EXTENSIBLE, plus nonfinite float rejection.
- Voice output channel-amplitude unity tests, plus empty sampler silence and old choke/retrigger/alias tests.
- Existing Windows MSVC strict CTest suite, actual packaged VST3 editor geometry smoke and Steinberg Validator must all pass on exact new source.
- Real user tests: choose PCM16 and PCM24/float32 samples; check row filename appears; other rows remain silent; compare actual WAV amplitude in DAW at full velocity; verify Windows Explorer -> row drag/drop and separately Studio Pro browser -> row drag/drop; test click, reconnect and reload.
- **Known M08 limitation:** kit file references/PCM and GUI names are not project-state persisted. No running light, finalized 125A visual skin or FLAC. No customer release/FINAL until those are implemented and approved.
