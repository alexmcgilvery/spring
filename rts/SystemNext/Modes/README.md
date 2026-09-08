# Mode concerns

Each concrete mode extends the common runtime interface through
[Mode.h](Mode.h). The bridge generates typed inputs from local compile-time
declarations. Concrete concern bodies receive no snapshot manager or global lookup.

| Mode | Input | Session | Display | Render |
|---|---|---|---|---|
| [SelectMenu](SelectMenu/SelectMenuMode.cpp) | Menu actions | Settings, startup decisions and handoff | Absent | Menu from Session |
| [LuaMenu](LuaMenu/LuaMenuMode.cpp) | Lua menu actions | Logical menu decisions and handoff | Visual menu maintenance | Menu and cursor |
| [PreGame](PreGame/PreGameMode.cpp) | Cancellation intent | Setup, connection and Loading handoff | Absent | Connection status |
| [Loading](Loading/LoadingMode.cpp) | Loading interaction | Progress, liveness, cancellation and Game handoff | Loading presentation | Intro/loading output |
| [Game](Game/GameMode.cpp) | Gameplay intent | Transport, authority, simulation and lifecycle decisions | Published world/client visuals | World, interface and capture |

Each `*Snapshots.h` contains local payload outlines and read declarations.
`Required` and `Optional` generate reference and nullable-pointer accessors.
The declaration is both a data-access restriction and a retention requirement;
it is not a mode capability flag.

Session is a logical concern in every current mode, including menus. Input does
not activate a replacement. An accepted Session request closes the old iteration;
the replacement receives a new activation generation and starts with its own Input.

Display is absent in SelectMenu and PreGame. Their Render declarations consume
Session.Current directly. Omitted concerns create no synthetic publications.
Other outline bodies explicitly return no publication, so downstream required
inputs remain unavailable until actual behavior is implemented.

Source briefs explain each input/history selection, outputs, authority, ordering,
headless behavior and retirement. They preserve local uncertainty about Lua,
GUI, loading and other adapters without allowing those dependencies to define the
target architecture. Mutable logical and visual resources remain separately owned;
any future concrete payload must be deeply immutable.
