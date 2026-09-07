# SystemNext — RFC-0 runtime module

Start with [GameRuntime.cpp](GameRuntime.cpp). The main loop lives directly in
SystemNext; GameRuntime.h declares its interface. Its current serial flow is:

```cpp
host.UpdatePlatformState();
const bool running = session.Advance();
auto guard = legacyFrame.AcquireGuard();
const bool frameReady = running && legacyFrame.PrepareAndDraw();
graphics.Present(frameReady);
```

Session advancement retains the authoritative network/demo pump and existing
simulation scheduling. Legacy frame execution still contains client preparation,
world/Lua/UI drawing and capture. Splitting those operations is subsequent work;
the loop must expose real operations as they become independently callable.

## Source layout

Folders describe major loop concerns. Legacy adapters live beside the concern
that they implement. Ordinary methods belong in .cpp files, with declarations
in corresponding .h files. Value records, trivial accessors and genuine generic
allocator code may remain in headers.

```text
SystemNext/
  GameRuntime.h/.cpp                 # main loop
  LegacyGameRuntime.h/.cpp           # constructs legacy bindings
  Platform/
    IPlatformHost.h                  # platform update contract
    LegacyPlatformHost.h/.cpp        # configuration, window, timer
  Simulation/
    ISimulationSession.h             # session advance contract
    LegacySessionPump.h/.cpp         # existing controller update
    LegacySimulationStateReader.h    # extraction interface, implementation pending
    Publication/
      PublishedSimFrame.h/.cpp       # owning simulation values and catalog
      SimFramePublicationStore.h/.cpp
      PublicationMemoryBudget.h/.cpp
    Observation/
      EntityIdentityRegistry.h/.cpp
      SimulationNotifications.h/.cpp
      SimulationEventJournal.h/.cpp
      event-classification.json
  Presentation/
    IPresentationFrame.h/.cpp         # legacy combined preparation/draw contract
    LegacyFrameExecutor.h/.cpp       # controller draw and load guard
  Graphics/
    IGraphicsPresenter.h             # present contract
    LegacyGraphicsPresenter.h/.cpp   # existing swap
  Diagnostics/
    BoundedTraceBuffer.h/.cpp
    JsonLinesTraceWriter.h/.cpp
    LegacyRuntimeDiagnostics.h/.cpp
  tools/
    analyze_runtime_log.py
    check_runtime_contracts.py
    test_runtime_tools.py
  hook-manifest.json
  sources.cmake
  tests.cmake
```

Rendering remains implemented by existing drawers; RFC-1 owns RenderingNext.
No empty renderer implementation is introduced here. Input/lifecycle remain in
SpringApp until their existing ordering can be migrated with evidence.

Public contracts and storage depend on standard-library values and other public
SystemNext contracts. Only Legacy-prefixed private adapters include engine
subsystems. The JSON writer implementation also uses the existing JSON library.
Compile adapters under each engine variant's definitions. Tests mirror the
concern folders in test/engine/SystemNext and use existing Catch2/CTest support.

The engine-owned hook manifest registers changes outside this module. The event
classification accounts for every legacy event, including synchronous callbacks.
The architecture and evidence tracker are in recoil-docs/vkfun-main-loop-rfc.md
and recoil-docs/work/rfc0. Engine validation does not require that checkout.

Status: implementation in progress. The serial loop and diagnostics have engine
bindings. Publication storage is tested independently; production extraction,
event observation and complete RFC-0 acceptance evidence remain outstanding.
