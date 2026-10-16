# Drumator milestone 02 — sample voice playback core

Scope: host-independent, realtime-bounded PCM voice renderer. **Not an audio file decoder, sampler GUI or VST3 instrument yet.**

## Audio/DSP contract
- Prepared immutable mono/stereo **planar float32 PCM**; explicit non-owning lifetime contract.
- Band-limited 32-tap Blackman-windowed sinc interpolation with 1024 fractional phases, and an anti-aliasing cutoff of min(1, 1/playbackRatio), expressed relative to source Nyquist.
- Kernel weights are normalized to unity DC gain. Kernel construction uses sin/cos and must execute OFF the audio thread.
- Pitch ratio = sourceRate/outputRate × 2^(semitones/12). Prepare a matching kernel off-thread for a change in playback ratio. Active voices retain their current kernel and sample.
- 64 concurrent voices (initial implementation cap); no hard voice steal. Further triggers are rejected with a count, not turned into discontinuous audio.
- Repeated hits on the same row overlap. Voice-end follows sample bounds. Mono channels are duplicated, stereo channels preserved.
- Amplitude = instrument linear gain × MIDI velocity/127. Constant-power panning (left cos, right sin, angle = (pan + 1) π/4).
- Choke group 0 disabled; positive groups fade previously active group voices when a valid group trigger starts. Default choke fade duration 2 ms is an **empirically chosen UX default**, not a physical hardware emulation; release length derives from the actual output sample rate and is configurable. Validate musically in host.
- On sample replacement, new triggers use the new row view, older voices retain the previous view and kernel. The eventual host bridge must delay reclaiming old PCM/kernel memory until NO voice still references it. **Do not ever free backing memory immediately upon swapping.**
- No clipping or automatic normalization is applied. A polyphonic sum above 0 dBFS is mathematically possible; proper gain staging is the caller's/user's responsibility.
- Audio callback: no heap allocations, file I/O, locks, unbounded resampler kernel preparation, or logging. Input event ordering and limits are validated up front. Buffers are zero-initialized by renderer after validation.

## Validation targets
- Native-pitch DC stability; hard-pan and stereo integrity.
- Retrigger overlap; muted row suppresses new triggers without killing existing voices.
- Sample replacement with already playing voices and retained immutable data.
- Choke fade reaches silence, active voices retire.
- Deterministic 64-voice capacity; invalid event vectors do not alter state or output buffers.
- Bandlimited octave-up alias suppression for an above-Nyquist source test tone, with a low-frequency control tone.
- Combined with milestone 01 sequencer timing regressions.

## Pending for release-quality audio
- Detailed stopband/transition-band sweeps at all required ratios and sample rates, numerical reference comparison with established resampling implementation, extreme performance tests, measured CPU p95/p99/max and sample-rate/block-size matrices.
- Host-thread safe asset ownership and the VST3 controller/processor transfer protocol.
- Actual WAV/FLAC/AIFF decoders and supported-format policy; project recall and missing asset relocation.
- Per-row settings state, host automation, stop/start semantics and multi-output bus routing.
- GUI, Steinberg VST3 SDK 3.8.1 integration, full 125A QA, Fender Studio Pro testing.

**No Windows/QA PASS is claimed by source code or documentation alone.** Read GitHub Actions job evidence.

## Verified evidence (2026-10-16)
- Source commit: `35c6436d88ea885d85d92864b97c8eabdfbfd106`.
- Official Windows Actions run: https://github.com/challanger2000/125A-Drumator/actions/runs/38906773944
- MSVC x64 Release build with `/W4 /WX`: PASS.
- CTest `drumator_core_tests`: PASS.
- CTest `drumator_voice_tests`: PASS.
- Total: **2/2 PASS**, zero failures.
- This only verifies host-independent C++ code with synthetic fixtures, **not** plugin host compliance, realtime pacing under DAW load, GUI, sample decoding, or final audio quality.
