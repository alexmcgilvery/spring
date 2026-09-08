# Immutable stage publications

[SnapshotReads.h](SnapshotReads.h) declares consumption at compile time.
[SnapshotView.h](SnapshotView.h) generates readable accessors.
[SnapshotManager.h](SnapshotManager.h) owns storage and invocation association.

```cpp
using SessionReads = SnapshotReads<
    Required<Stage::Input, Slot::Current>,
    Optional<Stage::Display, Slot::Previous>
>;

// In a concern receiving its generated view:
const auto& actions = snapshots.Input().Current();
const auto* visualFeedback = snapshots.Display().Previous();
```

Display.Current and undeclared sources/depths do not exist in that Session view.
Required visual feedback on logical stages is rejected because headless and
bootstrap execution must not wait for a visual producer.

Snapshot means owned immutable data. Publication adds producer, activation,
iteration, revision and optional completed-tick identity. A view owns leases to
its selected publications. A copied view or source window can outlive both its
invocation and the manager. Bare references require the owning view to remain alive.

Logical and visual IDs are independent monotonic names. Visual iteration 81 can
associate with logical iteration 96. BeginVisual selects the latest fully completed
logical iteration of the active mode. Its logical association never changes when
iteration 97 completes. Same-flow inputs resolve when a stage acquires its view,
so Render can read the Display just published in its own visual iteration.

Current names the associated producer invocation. Missing associated state remains
missing. Previous/Older select earlier committed publications, excluding the
associated invocation. Logical reads of visual history freeze at logical begin.
Simulation has an independent tick/revision cadence: its visual association names
the most recent completed simulation publication known to the selected logical
iteration, including when that iteration advances no tick. The original producer
identity remains intact.

Each producer's cached history depth derives from registered consumption, with
additional owning retention for open associations and views. Activations reset
mode-local history; application facts have independent continuity. Reload does not
create another accumulating history pool. Unbounded external leases can still
retain unbounded data: production capacity/overflow policy is not implemented.

Only an admitted Game Session can open simulation-publication scopes. This manages
completion identity and does not execute or authorize a tick. Simulation execution
remains Game-local. Session output alone can carry normal lifecycle requests.
Historical reads never replay commands, deliver lossless events, acknowledge
actions or apply a transition twice.

Admission and storage are implemented for serial scheduling. Retired activations
reject new invocations and commits. Already issued owning data stays readable.
The manager does not yet provide worker synchronization, asynchronous pacing or
production overload handling. Payload definitions must prohibit mutable aliases;
C++ constness on an arbitrary enclosing object cannot enforce deep immutability.
