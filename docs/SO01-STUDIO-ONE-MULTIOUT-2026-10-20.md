# SO01: Studio One 8 VST3 Multi-Out (20.10.2026)

**Owner reproduction:** Studio One's instrument output checklist enumerates `MAIN` and `DRUM 02`–`DRUM 16`, but toggling the additional outputs does not enable them. This is a host interaction bug, **not** a failure to discover the buses. Host UI screenshot supplied by the owner in chat.

## Root cause and external evidence

At source `7bc20aa4a618853fd202c317d02b01d66c54d011`, `Processor::initialize` called `addAudioOutput(name, kStereo, kAux, 0)` for the 15 secondary stereo outputs. `kAux` is a valid Steinberg VST3 bus type and works in some hosts. However, a Steinberg developer forum report explicitly documents Studio One's inability to activate multiple output buses unless all are `kMain`: https://forums.steinberg.net/t/importance-of-kmain-and-kaux-for-plugin-with-multiple-output-buses/733616 (Aug 2021). Independent real-world corroboration: Microtonic VST3 multi-out fails specifically in Studio One because its extra output buses are `kAux`: https://soniccharge.com/forum/topic/2909-microtonic-vst3-multi-out-not-working-in-studio-one .

Steinberg's normative bus documentation: https://steinbergmedia.github.io/vst3_doc/vstinterfaces/group__vstBus.html (multiple `kMain` buses permitted, listed before `kAux`; host activation independent of bus declaration).

## Scope of fix

- Only change the secondary `BusInfo::busType` from `kAux` to `kMain`.
- Keep bus 0 `MAIN` default active and 1..15 `DRUM 02`–`DRUM 16` with `flags=0` (optional/inactive at initial host instantiation).
- Preserve bus indices, stereo arrangement, names, internal row routing, fallback to MAIN when a stem is inactive, processor/controller class IDs, serialized kit/preset data, and real-time processing.
- Mandatory regression `SO01` added to the actual **packaged DLL host test**: assert 16 audio output buses, each stereo `kMain`, names, and only first default active. Existing `M27`, `M30`, `F08`, and `T02` check routing, simultaneous stems and 15 independent stereo signals.

## Verification and handoff

**Windows CI not yet verified for this candidate.** Do not label PASS before observing exact build SHA + test output + Steinberg Validator. Studio One 8 activation must be retested manually by owner after removing/replacing cached old plugin instance as appropriate (check in fresh song to avoid stale bus configuration), then routing e.g. KICK→MAIN and SNARE→DRUM 02. Only Studio One can confirm final host behavior. The official GPU/VSTGUI design chat should not change Processor bus metadata or assume it works automatically until this code is merged.

## Concurrent work constraint

The redesign of the VSTGUI continues independently. This focused correction modifies only `src/vst3/DrumatorProcessor.cpp`, `tests/PackagedAudioHostSmoke.cpp`, and this document. If the design branch moves during the work, reconcile its latest HEAD, do not reset or overwrite it.
