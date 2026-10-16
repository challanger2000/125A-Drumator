# Drumator milestone 01: deterministic sequencer core

This is a deliberately small and independently testable development milestone.
Status is source implementation; Windows Action and release results must be confirmed from evidence.

## Scope implemented
- Eight patterns, 16 rows each, 32 sixteenth-note steps; velocity 0/off or 1..127.
- Stateless triggering from the DAW-provided absolute musical position (PPQ) and block tempo.
- Integer sample offsets within half-open audio blocks: event exactly at the end belongs to the next block.
- Host stop, seek, negative PPQ and 32-step pattern wrap.
- Strict upper bound of 128 step boundaries per call; invalid/over-budget clocks emit no partial events.
- Versioned compact pattern state with transactional restore on malformed data.

## Boundary and limitations
- This is NOT YET an audible VST3 plugin. No sampler voices, sample decoding, GUI, bus mapping, or vintage effects yet.
- The core PatternBank is single-owner state; never mutate from UI while rendering. Implement explicit safe state handoff before VST3 GUI controls.
- No assumptions that non-blocking automatically means thread-safe. No file I/O, allocation, locks, or unbounded loops in the scheduler.
- Tempo is constant within the host block, as received from the host; no independently improvised clock.
- Fixed work budget is a protective guard against invalid/extreme host data, not a promised BPM limit.
- State v1 here covers the grid only; a full VST3 project state will need a versioned schema that also preserves sample references, names, routing and kit settings.
- Tests confirm deterministic scheduling only; they do not establish audible quality or host compliance.

## Next
Wire the pinned official Steinberg VST3 SDK 3.8.1. Build a measurable sample player, safe off-thread sample preparation and controller/processor handoff, then provisional VSTGUI and actual Windows plugin validation. Follow 125A-Engineering START-HERE and authoritative 125A-Branding instructions. Keep Windows x64 only and preserve main as the stable branch.
