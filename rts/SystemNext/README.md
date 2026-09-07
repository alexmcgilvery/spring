# SystemNext runtime and adapters

Legacy source is the reference implementation. Preserve its bodies, interfaces,
state ownership, input routing and execution path. Only recorded additive hooks
belong there. Adapter behavior and copied implementations belong in SystemNext.
Do not add friendship, remove methods or reverse-forward legacy methods to make
an adapter work. A missing connection is explicit work, not permission to change
that boundary.

The independently tested scheduling code describes:

```text
input → session → display → render → present
           └─ authoritative simulation steps
```

`ApplicationLoop`, `Session`, `ModeFrame` and `SerialVisualFrame` implement those
contracts with typed outcomes and invocation-owned visual state. The fake-host
tests exercise transitions, ordering and failure behavior. **The legacy application
is not redirected into this loop.** Its original Run/Update and input routes remain.

## Adapter availability

[Mode implementations](Modes/README.md) retain copied substantial operations.
They currently assume private controller access and lifetime APIs that legacy does
not provide. `systemnext_unavailable_adapter_sources` in `sources.cmake` explicitly
lists these uncompiled copies, along with host and simulation integration. This is
an incomplete adapter implementation, not a working alternative runtime. No
whole-file preprocessor blocks hide the copied statements.

Complete backing ownership and connections inside SystemNext before activating
these adapters. The legacy SimFrame implementation remains in place; the copy in
`Simulation/Simulation.cpp` does not drive legacy ticks.

## Observations and verification

Registered additive hooks record legacy game/network/lifecycle observations.
Runtime diagnostics use the engine write directory and a SystemNext-owned
`RuntimeLogFile`; opening/closing it does not change or broadcast through the legacy
file-sink API. Detailed records remain bounded and drain at the outer-loop boundary.

`tools/check_legacy_surface.py` verifies that the audited legacy files retain every
baseline source line. `tools/check_runtime_contracts.py` separately validates hook
presence and isolated dependency boundaries. The baseline manifest is local to the
engine; source archives without its Git revision cannot run the preservation check.

Publication/event storage and diagnostics have isolated tests. Complete production
extraction, adapter execution, graphics/capture qualification and independent
rendering remain unfinished.
