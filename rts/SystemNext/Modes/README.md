# Mode concern outlines

Read each implementation file as a description of expected behavior. Its functions
override the shared IMode concern interface, select their concrete context
alternative, and retain the expected-work documentation. Mode construction, cancellation and completion are
expectations inside the relevant concerns, not additional helper implementations.

| Mode | Input | Session | Display | Render |
|---|---|---|---|---|
| [SelectMenu](SelectMenu/SelectMenuMode.cpp) | Selection, settings, startup actions | Absent | Absent | Menu and pacing |
| [LuaMenu](LuaMenu/LuaMenuMode.cpp) | Lua menu interaction | Absent | Client maintenance and Lua updates | Eligibility, menu, cursor |
| [PreGame](PreGame/PreGameMode.cpp) | Cancellation | Setup, connection, loading handoff | Absent | Connection screen |
| [Loading](Loading/LoadingMode.cpp) | Responsiveness and input | Progress and completion | Pacing, lobby, intro maintenance | Loading/intro screen |
| [Game](Game/GameMode.cpp) | Interaction and submission | Jobs, transport, authoritative processing | Client state and preparation | World, interface, capture |

Absent concerns have no dummy function. [Game simulation](Game/Simulation/Simulation.cpp)
and its [publication](Game/Simulation/Publication/Publication.cpp) and
[observation](Game/Simulation/Observation/Observation.cpp) belong to Game. Mode-local
session/display/render work is not made shared merely because several modes have
similarly named blocks.

The conceptual input/session/display order does not imply that all legacy input
submission already occurs before session processing. Each outline records where
expected work may cross the intended boundary; later source annotation will make
that mapping precise. No conflict-resolution scheme is chosen here.

Each mode keeps its context declarations locally. Input contexts borrow the ordered
platform-event batch; they do not add a second queue. Session receives authority
needed by that mode. Display declares mutable client/preparation dependencies;
render receives its visual dependencies explicitly. All borrows end with the call
or earlier backing retirement. The application owns construction and rebinding.

All five modes inherit IMode. Constructor metadata states session capability and
display placement. Optional concerns absent from a mode are not declared there;
the base rejects attempts to schedule them. ModeContexts contains phase-specific
variants for the common interface, retaining each concrete dependency bundle.
ModeContextProvider builds those bundles at the application boundary, not inside
mode behavior or through a per-statement service facade.
