# 125A Drumator — development instructions

## Status
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
