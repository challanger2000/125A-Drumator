# Milestone 04 — WAV PCM16 parser groundwork

Status: independent decoder code and synthetic regression tests, NOT yet integrated into VST3.

- Strict RIFF/WAVE (not RF64/RIFX), PCM format 1, signed 16-bit mono/stereo.
- Validates chunk bounds, alignment, format, byte rate, and frame count; unknown chunks skipped with RIFF padding.
- Decodes entirely off the audio thread; no direct file access.
- Transactional result: failures preserve the prior decoded object.
- Float conversion uses the standard signed PCM divisor 32768; positive PCM maximum is 32767/32768, not 1.0.
- Additional WAV formats, sample rate conversion, host-safe asset handoff, file picker and project recall are pending.
- Never call this decoder from process().
- Tests are synthetic; fixed real WAV fixtures and independent decoder comparison still required before shipping.

## Explicit CI regression noted (2026-10-16)
The first WAV test build (commit `cb917e0`, run [#38915132444](https://github.com/challanger2000/125A-Drumator/actions/runs/38915132444)) failed because the workflow built only two previously named tests, then CTest attempted to run a third executable. Compiler build was successful; `drumator_wav_tests` was NOT built or run. This was a build-target-list omission, **not a WAV decoder PASS**. Milestone 05 replaces the brittle explicit test-target list with a dynamically generated aggregate `drumator_test_suite` target. Preserve the failure record.

## Confirmed correction
At source `8b2b41e`, Windows Actions [#38917602904](https://github.com/challanger2000/125A-Drumator/actions/runs/38917602904) built the WAV executable as part of `drumator_test_suite` and ran it: `drumator_wav_tests` **PASS**. Original omission run #38915132444 remains documented above. This is a synthetic decoder regression, not in-plugin sample import approval.
