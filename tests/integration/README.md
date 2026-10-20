# Windows controller/view/restore integration QA

Base: `f6f1933980f8d8c688c20c295cacda5a00a3a848` (product code `62f10a4`).
No version or release changes. These are Windows x64/MSVC/C++23 tests, not
Studio One tests. A written assertion is not a passed execution.

The existing `drumator_test_suite` now builds `drumator_integration_tests` when
`DRUMATOR_BUILD_VST3` and `BUILD_TESTING` are enabled on Windows. The existing
CTest command executes it. Packaged editor/audio tests remain independent and
load the actual assembled VST3 DLL.

## Deterministic probes without shipping test hooks

`Configure.cmake` reads the current source at configure time and generates copies
under `build/qa-source`. Exact anchors must occur once. It adds only:

- observation friendship in the Controller/View/Processor headers;
- pauses after the restore selection snapshot, inside the restore gate before
  model installation, and before selection CAS;
- pauses halfway through pattern replacement and after the reader's first
  generation load.

It does not replace decoding, commit logic, synchronization, rendering, native
views, focus teardown or state methods. The generated files use the real current
implementations and linked pinned SDK/VSTGUI. Probe waits and allocation faults
exist only in this test executable. They must never be interpreted as timing or
realtime performance measurements. The shipped `Drumator` target is unaffected.

The generated header's quoted sibling includes are resolved via the original
`src/core` directory. Only the pinned SDK root and `vstgui4` directory are SYSTEM
includes (`/external:I`, `/external:W0` under MSVC). Project and generated test
sources retain `/W4 /WX`; no product/SDK header is edited to silence warnings.

The test EXE initializes VSTGUI explicitly and provides a resource directory;
the packaged editor smoke separately checks native attachment/resource lookup
through the actual DLL. `IPlugView` methods are called through that public host
interface, as required by the pinned SDK (its concrete overrides are protected).

## Oracles

- T01: packaged component `getState` after actual PatternLength messages 32→16→32,
  compare stored tail cells of A–H/all rows; erased returned-step negative.
- T02: fifteen AUX channels with distinct float32 stereo codes and narrow justified
  tolerance; neighbor-bus swap and left/right swap must fail the same oracle.
- T03: pause a real writer halfway; coherent read must reject and unversioned
  atomic reads must yield a demonstrably mixed negative. Pause a real coherent
  reader, complete a write, then require rejection. Verify complete X/Y reads.
- T04: packaged two-kit stress with different sample bytes, patterns, gain,
  master, selected program and output assignment. Every audible block must match
  one complete four-channel signature; each of 24 restores must be acknowledged
  by new-kit audio and returned state. A separate source-instrumented integration
  test forces `process` inside a half-installed restore and requires exact gate
  silence, followed by the complete new sample/pattern/mix signature.
- O01: real `Processor::setState/process/getState` with a pause after the selection
  snapshot or during gate ownership. Test distinct, equal and ABA values with
  zero frames and ordinary blocks. ABA uses **two actual process calls** because
  SDK queues coalesce equal sample offsets. Later host events must survive the
  restore snapshot/CAS arbitration, including equal/ABA values. Returned state,
  controller/host label and audible pattern code must agree.
- Z01: override scalar/array `operator new` in the same executable as the actual
  Controller/SDK. Count real recall allocations and fault each reached position
  using fresh fixtures, two native views, live Undo/clipboard/stroke/text editor.
  Snapshot real private/public fields; rejection must preserve them and native
  focus, successful commit must invalidate old callbacks. DMR1/2/3 additionally
  fault each of the 16 default-name copies. Windows-internal allocation failures
  and aligned allocations are outside this allocator hook's scope.
- Z02: real attached frames, 100/150 user zoom at 100/150/200 host content scale,
  host refusals, saved bytes, reopen and preflight rejection of two-view changes.
  The packaged DLL separately exercises native attach, recall and refusal.
- Z03: real EDIT HWND, pending text, corrupt/accepted 16→4 recall, old live-listener
  callbacks, Enter/Escape/blur, two views, HWND destruction and editor removal.

Always-active exceptions/checks remain active in Release builds. Every failing
case fails CTest; no known product failure is classified as expected success.
`GITHUB_STEP_SUMMARY` records executed outcomes, including failures. Full MSVC
logs and exact run/commit remain required to diagnose compile failures. Source
checks and CMake script-only generation are not Windows/C++ test executions.
