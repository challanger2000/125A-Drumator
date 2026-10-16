# 125A Drumator

Vintage-inspired multi-format drum sampler with a 32-step running-light sequencer. Focus: **drop a sample onto a drum row, program steps, play.**

**M08 Windows-verified technical candidate (2026-10-16):** removes the misleading internal kick (empty sample rows are silent), supports mono/stereo WAV PCM 8/16/24/32-bit, IEEE float32 and common WAVE_FORMAT_EXTENSIBLE variants, and keeps recorded level at center pan (unity per channel, no normalization). GUI distinguishes load failure causes, and VSTGUI file-path drag/drop targets individual rows. **CI VERIFIED; real Studio Pro loading and dragged-file confirmation remain unverified until user test.** No release or FINAL label.

M08 **verified Windows QA** at source commit `73b0430c673ea86d7133ca6ebd75a56c7515c3bd`, [Actions #38952896083](https://github.com/challanger2000/125A-Drumator/actions/runs/38952896083) **SUCCESS**: MSVC Windows x64 compiled, six CTests **6/6 PASS**, official Steinberg Validator **47/47 PASS**, actual packaged editor smoke **1100×600 PASS**. Delivered artifact `125A-Drumator-M08-Windows-x64-WAV-Drop-Audition` (ID 12069382610): ZIP was downloaded, its three expected paths inspected, PE x64 confirmed, `moduleinfo.json` and `Drumator.uidesc` parsed as valid JSON, ZIP integrity clean. In Studio Pro user tests for PCM24/float WAV loading and OS/browser drag/drop **remain unverified**.

**Status:** development; **first VSTGUI prototype built in Windows CI, not yet DAW-tested**. 16x32 clickable step grid, pattern buttons and PCM16 WAV picker exist in source, but independent DAW GUI/audio behavior and restored sample references are NOT verified. **NOT a Drumator release.**

**GUI resource fix / verified source:** commit `7097bc5a9a6fa6178ffde7fa706aa9be94de280c`, Windows Actions [#38944959865](https://github.com/challanger2000/125A-Drumator/actions/runs/38944959865), 6/6 CTests PASS, Steinberg Validator 47/47 PASS, and new **editor geometry smoke 1100 × 600 PASS** on the packaged VST3. Downloaded artifact `125A-Drumator-M07-Windows-x64-GUI-PROTOTYPE` (ID 12064249354) has been independently ZIP-verified. Studio Pro GUI rendering and click/WAV playback are NOT yet confirmed.

**Previous, faulty M07 Windows package (DO NOT INSTALL):** [GitHub Actions #38943169799](https://github.com/challanger2000/125A-Drumator/actions/runs/38943169799), source commit `e071e5892c6d68771318559931881d501d8a0256`: **Windows x64 build PASS, six CTests 6/6 PASS, Steinberg Validator 47/47 PASS**. Artifact `125A-Drumator-M07-Windows-x64-GUI-PROTOTYPE` (ID `12063164348`) independently downloaded and ZIP-structure verified: **`125A Drumator.vst3/Contents/x86_64-win/125A Drumator.vst3`**, resource files present, strict moduleinfo JSON valid. Not yet manually Studio Pro tested and NOT FINAL.

**Verified M06 technical build:** source `9f223c0f6c4d8e65a8ce368afd08a0387643af57`, [GitHub Actions #38919501529](https://github.com/challanger2000/125A-Drumator/actions/runs/38919501529) **SUCCESS**: Windows MSVC x64 VST3 build PASS; five CTests **5/5 PASS**; moduleinfotool PASS; official Steinberg Validator **47/47 PASS** on assembled bundle. Engineering-only artifact `Drumator-M06-INTERIM-VST3-UNTESTED` (ID `12053822596`). No DAW WAV-import/GUI/audio test or final release.

**Previous verified technical milestone:** Verified at source commit `8b2b41eb1feb8dca09a325e7d649841a34ce58af`, [Windows Actions #38917602904](https://github.com/challanger2000/125A-Drumator/actions/runs/38917602904): MSVC x64 Release build PASS; CTest **4/4 PASS** (`drumator_core_tests`, `drumator_voice_tests`, `drumator_wav_tests`, `drumator_exchange_tests`); official moduleinfotool creation and validation PASS; assembled Windows VST3 bundle official Steinberg Validator **47/47 PASS**. Engineering-only artifact `Drumator-M03-VST3-UNTESTED`, id `12052789021` (still temporary synthesized kick and no editor).

**Latest Windows evidence:** [Actions #38923496070](https://github.com/challanger2000/125A-Drumator/actions/runs/38923496070) / compiled commit `d1055c1b59aba15b6a7c9d594e5f882b13fd630d`: **6/6 CTest PASS**, Steinberg Validator **47/47 PASS**, engineering-only GUI candidate ZIP available. Not manual host tested.

Development: `develop/v1.0.0`.

- [SDK baseline](docs/SDK-BASELINE.md) — official Steinberg VST3 SDK **3.8.1**, pinned tag and commit
- [Milestone 01](docs/MILESTONE-01.md) — deterministic step scheduler and state storage
- [Milestone 02](docs/MILESTONE-02.md) — sample voices, choke, pitch interpolation and tests
- [Milestone 03](docs/MILESTONE-03.md) — pinned SDK integration and verifiable Windows VST3 prototype
- [Milestone 04](docs/MILESTONE-04.md) — strict WAV PCM16 decoder (test coverage under verification)
- [Milestone 05](docs/MILESTONE-05.md) — prepared-sample ownership handoff and deferred reclamation
- [Milestone 06](docs/MILESTONE-06.md) — candidate VST3 sample-message receiver and 16-row MIDI audition; no GUI yet
- [Milestone 07](docs/MILESTONE-07.md) — VSTGUI 16-row/32-step prototype, initial WAV file chooser and atomic step state (CI/host verification pending)
- [V1 concept/scope](docs/V1-SCOPE.md) — user workflow and technical requirements
- [Contributor/agent rules](AGENTS.md) — mandatory 125A Engineering entry and Windows-only policy

Engineering authority: [125A-Engineering / START-HERE.md](https://github.com/challanger2000/125A-Engineering/blob/main/START-HERE.md).
