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
