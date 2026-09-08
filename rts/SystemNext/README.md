# Application runtime contracts

The application loop executes logical and visual work serially, with independent
iteration identities:

```text
input → session → authoritative Game steps and publications
       ↓ accepted lifecycle decision
display → render → present, when the application schedules visual output
```

[ApplicationLoop.cpp](ApplicationLoop.cpp) owns scheduling. Its stage methods take
logical or visual iteration IDs. The [snapshot manager](Globals/Snapshots/SnapshotManager.h)
selects immutable inputs; [typed mode dispatch](Modes/Mode.h) exposes only each
concern's declared reads. [Mode contracts](Modes/README.md) describe the expected
behavior and why it belongs in each stage.

Input publishes interpreted actions. Only Session output can carry a normal mode
switch, reload or exit request. Lifecycle constructs a replacement and commits
activation after Session returns. The replacement begins its own logical iteration
and input batch. Startup, OS exit and fatal failure remain application responsibilities.

Visual work consumes one frozen logical association. The application owns the
backend, synchronization, rendered-output lifetime and presentation. A headless
application has no visual subsystem and does not acquire visual contexts or invoke
Display, Render or Present. The current scheduler remains serial and blocking.

Implemented infrastructure includes typed read declarations, generated views,
consumer-derived history, owning leases, activation admission, lifecycle commitment,
and exact-output presentation receipts. These mechanisms are validated with fake
modes and adjacent services.

Concrete mode bodies, input adapters, simulation execution/extraction, detailed
payload schemas and diagnostic recording remain documented outlines. Their explicit
no-publication results must not be mistaken for working mode behavior. The production
engine does not call this loop yet.

Shared state belongs in [Globals](Globals/README.md) only when it is application-wide.
Game simulation, observation and publication schemas stay beneath Game. Source
briefs define contracts; legacy links identify later investigation sources.
