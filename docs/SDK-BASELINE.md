# Official VST3 SDK baseline — 125A Drumator

Verified on **2026-10-16** against the official Steinberg developer portal and GitHub repository.

- Repository: https://github.com/steinbergmedia/vst3sdk
- Official release: **VST3 SDK 3.8.1** (Steinberg change log dated 2026-08-11)
- Immutable tag: `v3.8.1_build_84`
- Pinned top-level commit: `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`
- Documentation: https://steinbergmedia.github.io/vst3_dev_portal/pages/Versions/Version+3.8.1.html
- Target: Windows x64 VST3 / MSVC. VSTGUI comes from the SDK's tagged `vstgui4` submodule.

## Required integration discipline
Use the exact tagged source **recursively**, including `base`, `cmake`, `pluginterfaces`, `public.sdk`, and `vstgui4` submodules. Do not substitute a moving branch or automatically upgrade individual submodules.

Reproducible source retrieval, if a local/CI checkout is needed:

```bash
git clone --recursive --branch v3.8.1_build_84 https://github.com/steinbergmedia/vst3sdk.git
cd vst3sdk
git rev-parse HEAD
git submodule status --recursive
```

Expected top-level HEAD: `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`.
Validate each submodule commit from the tagged SDK checkout before compiling. Fix this dependency baseline in the future build configuration; no silently floating `HEAD`.

## Current state
**SDK VERSION PIN DOCUMENTED; SDK INTEGRATION/BUILD NOT YET IMPLEMENTED OR TESTED.**
Before writing CMake targets or using an unfamiliar API, inspect the exact SDK revision and official examples. Implement actual integration on `develop/v1.0.0` and verify under Windows CI; no untested build claim.

Official primary sources:
- https://github.com/steinbergmedia/vst3sdk/tree/v3.8.1_build_84
- https://steinbergmedia.github.io/vst3_dev_portal/
