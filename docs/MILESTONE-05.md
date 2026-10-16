# Drumator milestone 05 — prepared sample ownership and audio-thread exchange

Status: implementation candidate, Windows CI and full VST3 integration **NOT VERIFIED** until a specific workflow run succeeds. This code is a separately testable bridge, not yet a front-panel loading feature.

## Data flow and ownership contract
1. On a NON-AUDIO worker/control thread, read a WAV from disk (future VSTGUI/file browser; deliberately not implemented here).
2. Decode its bytes with strict PCM16 WAV decoder; prepare the immutable PCM and 32-tap/1024-phase sinc kernel for the current host output rate and fixed pitch.
3. Publish one exclusive prepared sample object for a row using release/acquire atomic ownership transfer. A producer cannot overwrite an occupied pending update. No malloc, decoding or file I/O occurs on the audio thread.
4. At the beginning of an audio block, adopt the prepared object only after atomically taking pointer ownership. A wrong-output-rate object is rejected and returned through deferred reclamation.
5. Active voices retain old PCM/kernel pointers plus an identity token. Replaced row samples do NOT kill their existing one-shots, and no old asset is reclaimed while the voice engine still references it.
6. After rendering, the audio thread only *enqueues pointers* to assets that are no longer in use. A bounded, lock-free SPSC audio->worker queue transports them; only the worker actually destroys them.
7. If return queue/retire slots are full, deferred work is retained and updates are backpressured; never free audio assets to make room or silently steal a sounding voice.
8. Exchange destruction is allowed only after BOTH audio and worker threads have stopped. Do not treat this single-producer/single-consumer protocol as safe for multiple simultaneous loader threads.

## Constraints and exceptions
- A sample version belongs to ONE row: no pointer-sharing across rows unless a future explicit multi-owner contract is implemented.
- Ownership and lifetime protocol is standalone C++; it is **not yet invoked by the VST3 Processor** and has no GUI/file-drop integration. Milestone 03 continues to use its explicitly documented synthesized-kick test fixture.
- Currently only RIFF/WAV signed PCM16 mono/stereo; malformed or unsupported WAV leaves previously prepared asset unchanged. No FLAC, AIFF, MP3 or OGG claims yet.
- Changing host output sample rate requires retiring/repreparing current assets before using their old sample-rate-specific kernels. The first VST3 integration must define the deactivation/reset and update protocol.
- Pattern/sample state and portable file references need a separate versioned VST3 project-state design. Never serialize raw pointers.
- Audio allocation profiling, paced realtime p95/p99/max deadline tests, rapid concurrent sample-replacement stress, real-audio fixtures, and the actual DAW/UI remain future gates.
- A protected current sample and retained old voice may remain live even when replacement is delayed; this is deliberate safe backpressure, not a release-complete lifecycle.
- The output amplitude of overlapping one-shots can exceed 0 dBFS. No undocumented gain compression or clipping is inserted.

## Regression coverage under test
- Last-row publication and first stereo/single-row rendering;
- transactionally rejected malformed WAV and invalid sample-rate/pitch;
- release/acquire publication and rejection of competing row updates;
- withdrawal of an as-yet-unadopted asset;
- rejection and worker-side reclamation of mismatched output-rate assets;
- replace while old PCM is still sounding (both old/new hearable);
- deferred reclaim only after all matching voices finish;
- exact source PCM16 amplitude and hard pan;
- existing core + voice tests alongside new WAV decoder test.

## CI prevention
Build through the generated aggregate CMake target \`drumator_test_suite\`, not a hand-maintained list of test executables; CTest runs all registered tests afterwards. The prior run #38915132444, which failed due to the omitted WAV executable, remains documented. Avoid redundant full SDK rebuilds until code and tests are coherent.

No claims of VST3 GUI, file loading, host audio QA, real-time safety measurement, 125A Plugin Tester PASS, Fender Studio Pro test, or FINAL.
