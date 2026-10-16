# 125A Drumator

Vintage-inspired multi-format drum sampler with a 32-step running-light sequencer. Focus: **drop a sample onto a drum row, program steps, play.**

**Status:** development; **Windows x64 VST3 technical prototype exists**, still with only a temporary internally generated kick and no editor/sample file loading. **NOT a Drumator release.**

**Verified integration milestone:** [Windows Actions #38913822453](https://github.com/challanger2000/125A-Drumator/actions/runs/38913822453), compiled source commit `6bf1835693e4271a2814086116924c6d08de2c59`: MSVC x64 build PASS; standalone CTest 2/2 PASS; official Steinberg Validator 47/47 PASS on the **assembled bundle**; module metadata validation PASS. Artifact `Drumator-M03-VST3-UNTESTED` is for engineering integration only, not Fender Studio Pro-tested.

Development: `develop/v1.0.0`.

- [SDK baseline](docs/SDK-BASELINE.md) — official Steinberg VST3 SDK **3.8.1**, pinned tag and commit
- [Milestone 01](docs/MILESTONE-01.md) — deterministic step scheduler and state storage
- [Milestone 02](docs/MILESTONE-02.md) — sample voices, choke, pitch interpolation and tests
- [Milestone 03](docs/MILESTONE-03.md) — pinned SDK integration and verifiable Windows VST3 prototype
- [Milestone 04](docs/MILESTONE-04.md) — strict WAV PCM16 decoder (test coverage under verification)
- [Milestone 05](docs/MILESTONE-05.md) — prepared-sample ownership handoff and deferred reclamation
- [V1 concept/scope](docs/V1-SCOPE.md) — user workflow and technical requirements
- [Contributor/agent rules](AGENTS.md) — mandatory 125A Engineering entry and Windows-only policy

Engineering authority: [125A-Engineering / START-HERE.md](https://github.com/challanger2000/125A-Engineering/blob/main/START-HERE.md).
