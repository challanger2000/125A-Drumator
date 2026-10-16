# 125A Drumator

Vintage-inspired multi-format drum sampler with a 32-step running-light sequencer. Focus: **drop a sample onto a drum row, program steps, play.**

**Status:** development; **Windows x64 VST3 technical prototype exists**, still with only a temporary internally generated kick and no editor/sample file loading. **NOT a Drumator release.**

**Verified M06 technical build:** source `9f223c0f6c4d8e65a8ce368afd08a0387643af57`, [GitHub Actions #38919501529](https://github.com/challanger2000/125A-Drumator/actions/runs/38919501529) **SUCCESS**: Windows MSVC x64 VST3 build PASS; five CTests **5/5 PASS**; moduleinfotool PASS; official Steinberg Validator **47/47 PASS** on assembled bundle. Engineering-only artifact `Drumator-M06-INTERIM-VST3-UNTESTED` (ID `12053822596`). No DAW WAV-import/GUI/audio test or final release.

**Previous verified technical milestone:** Verified at source commit `8b2b41eb1feb8dca09a325e7d649841a34ce58af`, [Windows Actions #38917602904](https://github.com/challanger2000/125A-Drumator/actions/runs/38917602904): MSVC x64 Release build PASS; CTest **4/4 PASS** (`drumator_core_tests`, `drumator_voice_tests`, `drumator_wav_tests`, `drumator_exchange_tests`); official moduleinfotool creation and validation PASS; assembled Windows VST3 bundle official Steinberg Validator **47/47 PASS**. Engineering-only artifact `Drumator-M03-VST3-UNTESTED`, id `12052789021` (still temporary synthesized kick and no editor).

Development: `develop/v1.0.0`.

- [SDK baseline](docs/SDK-BASELINE.md) — official Steinberg VST3 SDK **3.8.1**, pinned tag and commit
- [Milestone 01](docs/MILESTONE-01.md) — deterministic step scheduler and state storage
- [Milestone 02](docs/MILESTONE-02.md) — sample voices, choke, pitch interpolation and tests
- [Milestone 03](docs/MILESTONE-03.md) — pinned SDK integration and verifiable Windows VST3 prototype
- [Milestone 04](docs/MILESTONE-04.md) — strict WAV PCM16 decoder (test coverage under verification)
- [Milestone 05](docs/MILESTONE-05.md) — prepared-sample ownership handoff and deferred reclamation
- [Milestone 06](docs/MILESTONE-06.md) — candidate VST3 sample-message receiver and 16-row MIDI audition; no GUI yet
- [V1 concept/scope](docs/V1-SCOPE.md) — user workflow and technical requirements
- [Contributor/agent rules](AGENTS.md) — mandatory 125A Engineering entry and Windows-only policy

Engineering authority: [125A-Engineering / START-HERE.md](https://github.com/challanger2000/125A-Engineering/blob/main/START-HERE.md).
