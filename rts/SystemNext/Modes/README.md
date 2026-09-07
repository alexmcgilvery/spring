# Mode adapter copies

SelectMenu, LuaMenu, PreGame, Loading and Game retain their copied input, session,
display and render flows in substantial functions. These are **not currently
compiled or connected adapters**. The original legacy controllers remain intact.

The former integration depended on added friendship, controller instance/generation
APIs, modified input routes and reverse-forwarded legacy entry points. Those legacy
changes have been removed. Local `ADAPTER-*-BACKING` documentation identifies the
missing state/access connection. `sources.cmake` explicitly lists unavailable
translation units rather than presenting engine builds as mode validation.

| Adapter | Missing connection |
|---|---|
| SelectMenu | Own GUI/input backing and callback lifetime without modifying SelectMenu |
| LuaMenu | Own menu maintenance/timing state and lifecycle connections |
| PreGame | Setup/task backing, cancellation and retirement; loading/save handoff |
| Loading | Loading/progress state, reentrant queue and retirement ownership |
| Game | Session/display/input state and private helper dependencies |

Shared scheduling and typed-result tests remain useful, but do not validate these
copies. Original local PRE/LOAD/SIM conflict blocks are retained for later adapter
work. They no longer stop the running legacy application.

`ADAPTER-BINDING` also requires SystemNext-owned activation identity; observing only
an unmodified legacy controller address cannot establish same-address reuse safety.
`ADAPTER-HOST` requires a host connection without private SpringApp access. The
simulation copy requires its own backing contract before it may become authoritative.
