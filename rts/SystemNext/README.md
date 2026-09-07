# SystemNext runtime

Start with [ApplicationLoop.cpp](ApplicationLoop.cpp). Source implementations and
their documentation blocks define the current behavior and its rationale. This
overview is a navigation aid, not a separate source of runtime policy.

```text
[ input ] -> [ session ] -> [ simulation ]
                                  |
                           published state
                                  v
                           [ presentation ]
                                  |
                            render snapshot
                                  v
                            [ rendering ]
                                  v
                             [ present ]
```

These are responsibility boundaries, not six once-per-iteration calls. Session
service processes commands and permitted ticks in authoritative stream order.
The active mode supplies session behavior when applicable. Client modes without
a session update through presentation. Visual work still runs synchronously and
blocks subsequent session service; no concurrent-rendering capability is claimed.

## Execution entry points

- `ApplicationLoop::Run` owns repetition. Initialization and shutdown surround it.
- `RunIteration` services watchdog/input/save, selects reload or update/draw, then
  drains diagnostics. A mid-iteration quit request does not truncate that work.
- `ServiceSession` dispatches only modes that handle a session. Its named result
  destructures into application continuation and visual context.
- `SerialVisualFrame::ExecuteFrame` updates a client-only mode when required,
  acquires loading synchronization, decides drawing eligibility, and presents.
  An update exit request suppresses drawing but still reaches the swap function.

Session and visual bindings resolve the current controller at their actual call
sites. A controller replaced during update is drawn without being updated twice.
A frame context contains only iteration facts, not live pointers or a snapshot.

## Owned mode preparation

[Modes/README.md](Modes/README.md) introduces the five one-to-one research
skeletons. Their source briefs record confirmed constraints; their session methods
remain abstract and they are not registered with the loop. Visual/service APIs
will be defined after each mode's data, ordering and lifetime review.

## Source layout

```text
SystemNext/
  ApplicationLoop.h/.cpp                 # scoped main loop
  LoopServices.h                        # named concern contracts and binding bundle
  Lifecycle/
    LegacyLoopServices.h/.cpp            # one-time bindings to existing host operations
  Session/
    IRuntimeMode.h                      # mode-provided session behavior
    Session.h                           # authoritative session advancement contract
    SessionUpdate.h/.cpp                 # destructured application/visual facts
    LegacySession.h/.cpp                 # game/loading session adapters and mode mapping
  Simulation/
    LegacySimulationStateReader.h       # extraction interface; implementation pending
    Publication/                        # owned state, catalogs, bounded frame storage
    Observation/                        # identity, notifications and event retention
  Presentation/
    IVisualFrame.h                      # visual execution boundary
    SerialVisualFrame.h/.cpp             # serial eligibility and guarded visual scope
    LegacyVisualFrame.h/.cpp             # existing draw and present bindings
  Diagnostics/
    PhaseToken.h
    LoopPhaseScope.h/.cpp                # balanced loop observation
    BoundedTraceBuffer.h/.cpp
    JsonLinesTraceWriter.h/.cpp
    LegacyRuntimeDiagnostics.h/.cpp
  tools/                                # offline validation and analysis
  hook-manifest.json
  sources.cmake
  tests.cmake
```

Ordinary implementation belongs in .cpp files. Public contracts depend on copied
values and other public contracts. Legacy-prefixed adapters may include required
engine types and compile separately with each engine variant's definitions.

Presentation preparation, rendering and capture remain combined inside existing
drawing implementations. Graphics device ownership and independent scheduling
remain future work. There are no empty renderer implementations or pretend
snapshot-consuming adapters here.

The source checker validates hook presence, event classification and dependency
boundaries. Runtime source contracts must stand alone without design-document
references. Tests under `test/engine/SystemNext` use existing Catch2/CTest support.

Publication storage and notification retention have isolated tests; production
extraction and complete observation coverage are still under implementation.
