# Immutable publications

[SnapshotReads.h](SnapshotReads.h) declares consumption at compile time,
[SnapshotView.h](SnapshotView.h) generates readable accessors, and
[SnapshotManager.h](SnapshotManager.h) owns storage and invocation association.

```cpp
using DisplayReads = SnapshotReads<
    Required<Stage::Window, Slot::Current>,
    Required<Stage::GraphicsOutput, Slot::Current>,
    Required<Stage::Session, Slot::Current>
>;

const auto& window = snapshots.Window().Current();
const auto& target = snapshots.GraphicsOutput().Current();
const auto& session = snapshots.Session().Current();
```

Undeclared sources and slots do not exist in a generated view. Required visual
feedback on logical stages is rejected because headless and bootstrap work cannot
wait for a visual producer.

A snapshot is owned immutable data. A publication adds producer, activation,
iteration, revision and optional completed-tick identity. Views own leases to
their selections and can outlive their invocation, activation and manager.

PlatformInput, Window and GraphicsOutput are application-owned publication
families. Their history survives mode activation, while mode-produced histories
begin fresh for each activation generation. PlatformInput and Window are sampled
together at logical begin. GraphicsOutput is selected at visual begin. These are
sources, not extra execution stages.

The manager allocates independent monotonic logical and visual IDs. Beginning
visual work selects the latest fully completed logical iteration for the active
mode and freezes all cross-flow inputs. Same-flow Current resolves at acquisition,
allowing Render to consume Display from its visual iteration. Later publications
cannot change an acquired view.

Current names the explicitly associated producer invocation. Missing Current
remains missing. Previous, Older and History select earlier publications without
promoting unrelated latest state. Simulation has an independent tick/revision
cadence and retains its original producer identity across no-tick iterations.

History depth derives from registered readers. Retired activations reject new
admission and commits, while issued leases remain readable. This serial manager
does not yet implement worker synchronization, asynchronous pacing or production
overload handling. Concrete payloads must own their values and prohibit mutable
aliases.
