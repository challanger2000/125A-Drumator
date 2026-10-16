# 125A Drumator — development instructions

## Status
M09 under development: 4 visible rows by default, + expand / guarded minus, DMR3 row-count project state, packaged VST3 host audio smoke. **No Windows M09 PASS claimed until a matching run succeeds.** Prior verified M08 results remain below.

M08 **verified Windows QA** at source commit `73b0430c673ea86d7133ca6ebd75a56c7515c3bd`, [Actions #38952896083](https://github.com/challanger2000/125A-Drumator/actions/runs/38952896083) **SUCCESS**: MSVC Windows x64 compiled, six CTests **6/6 PASS**, official Steinberg Validator **47/47 PASS**, actual packaged editor smoke **1100×600 PASS**. Delivered artifact `125A-Drumator-M08-Windows-x64-WAV-Drop-Audition` (ID 12069382610): ZIP was downloaded, its three expected paths inspected, PE x64 confirmed, `moduleinfo.json` and `Drumator.uidesc` parsed as valid JSON, ZIP integrity clean. In Studio Pro user tests for PCM24/float WAV loading and OS/browser drag/drop **remain unverified**.

**M08 source candidate (2026-10-16):** removes the misleading internal kick (empty sample rows are silent), supports mono/stereo WAV PCM 8/16/24/32-bit, IEEE float32 and common WAVE_FORMAT_EXTENSIBLE variants, and keeps recorded level at center pan (unity per channel, no normalization). GUI distinguishes load failure causes, and VSTGUI file-path drag/drop targets individual rows. **CI, real Studio Pro acceptance and dragged-file confirmation remain unverified until tested.** No release or FINAL label.

**GUI resource fix / verified source:** commit `7097bc5a9a6fa6178ffde7fa706aa9be94de280c`, Windows Actions [#38944959865](https://github.com/challanger2000/125A-Drumator/actions/runs/38944959865), 6/6 CTests PASS, Steinberg Validator 47/47 PASS, and new **editor geometry smoke 1100 × 600 PASS** on the packaged VST3. Downloaded artifact `125A-Drumator-M07-Windows-x64-GUI-PROTOTYPE` (ID 12064249354) has been independently ZIP-verified. Studio Pro GUI rendering and click/WAV playback are NOT yet confirmed.

**Do not substitute validator PASS for UI QA.** When VSTGUI is in a separately compiled static target and Windows bundle mode is disabled, compile bundle resource-base support into that target as well and run the real editor geometry smoke test on the final installed structure.

Windows CI VERIFIED at source commit d1055c1b59aba15b6a7c9d594e5f882b13fd630d, Actions #38923496070: Windows x64 MSVC build PASS, CTest 6/6 PASS, official Steinberg Validator 47/47 PASS on assembled standard Windows VST3 bundle, moduleinfotool PASS; uploaded engineering-only artifact Drumator-M07-GUI-PROTOTYPE-UNTESTED (ID 12054908672). No actual Fender Studio Pro editor/interactive WAV import test, 125A Plugin Tester, sample file project recall or FINAL release.

Milestone 07 GUI source candidate: 16x32 grid, WAV chooser and versioned atomic step state **not yet Windows-built/DAW verified**. Prior Milestone 06 test results remain separately documented below. Sample file contents and references are not yet restored from host state.

**Verified M06 technical build:** source `9f223c0f6c4d8e65a8ce368afd08a0387643af57`, [GitHub Actions #38919501529](https://github.com/challanger2000/125A-Drumator/actions/runs/38919501529) **SUCCESS**: Windows MSVC x64 VST3 build PASS; five CTests **5/5 PASS**; moduleinfotool PASS; official Steinberg Validator **47/47 PASS** on assembled bundle. Engineering-only artifact `Drumator-M06-INTERIM-VST3-UNTESTED` (ID `12053822596`). No DAW WAV-import/GUI/audio test or final release.

DEVELOPMENT / Verified at source commit `8b2b41eb1feb8dca09a325e7d649841a34ce58af`, [Windows Actions #38917602904](https://github.com/challanger2000/125A-Drumator/actions/runs/38917602904): MSVC x64 Release build PASS; CTest **4/4 PASS** (`drumator_core_tests`, `drumator_voice_tests`, `drumator_wav_tests`, `drumator_exchange_tests`); official moduleinfotool creation and validation PASS; assembled Windows VST3 bundle official Steinberg Validator **47/47 PASS**. Engineering-only artifact `Drumator-M03-VST3-UNTESTED`, id `12052789021` (still temporary synthesized kick and no editor). The PCM16 WAV decoder and sample exchange are host-independent tested modules. A **new processor message receiver and MIDI audition adapter are currently under CI validation**, and there is still no user file picker/drag-drop GUI or end-to-end DAW sample-import evidence. No 125A Plugin Tester, real DAW audio/UI QA, owner approval, or FINAL release exists.

## Mandatory first read
Before *any* implementation, audit, DSP, GUI, build or release work:
1. Open `challanger2000/125A-Engineering/START-HERE.md`.
2. Follow its complete reading order and apply all relevant Engineering, Standards, QA, Research and Roadmap documents.
3. Read the current `challanger2000/125A-Branding/README.md` and authoritative master assets before any branded GUI/logo work. Use approved 125A-Knob-Designer assets where applicable.

## Project policy
- Windows 11 x64, VST3 instrument only. No macOS or Linux builds or releases.
- Development branch: `develop/v1.0.0`. Protect stable `main`; no casual implementation directly on `main`.
- Official Steinberg VST3 SDK **3.8.1**, tag `v3.8.1_build_84`, commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`, including its **exact tagged submodules**. See `docs/SDK-BASELINE.md`.
- Never use a floating SDK `master`, a guessed SDK API, or separately update VSTGUI without verification.
- Prefer established audio/DSP theory and exact Steinberg VST3 contracts, reproducible measurement and realistic QA; no speculative workarounds or arbitrary thresholds.
- No file I/O, dynamic allocation, blocking locks, or unbounded work inside the audio callback.
- Static product design precedes the **final** branded GUI. Early functional prototype may use plain VSTGUI.
- Keep the user-facing workflow deliberately simple. The current concept is documented in `docs/V1-SCOPE.md`.
- Before any write: verify actual repository, branch HEAD, version and affected files; avoid unnecessary GitHub Actions runs.
- Be exact about status: concept is not implemented; compilation is not QA; QA is not owner-approved release.
