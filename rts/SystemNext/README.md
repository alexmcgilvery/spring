# Snapshot-driven application runtime

The application loop executes logical and visual work serially with independent
iteration identities:

```text
application lifecycle
    input → session → authoritative Game steps and publications
    display → render → present, when Graphics schedules visual output
application lifecycle and diagnostics
```

[ApplicationLoop.cpp](ApplicationLoop.cpp) owns scheduling and the active mode
binding. It has explicit private dependencies on Platform, ApplicationLifecycle,
SnapshotManager, Diagnostics and optional Graphics. Modes cannot access those
application executors. They receive only their compile-time declared immutable
snapshot views.

Application ownership is organized by responsibility:

- [Platform](Application/Platform/Platform.h) owns native event collection and
  window lifetime and publishes separate input and window facts.
- [Graphics](Application/Graphics/Graphics.h) owns visual selection, render
  execution, target synchronization, output lifetime and presentation.
- [Lifecycle](Application/Lifecycle/ApplicationLifecycle.h) initializes the
  application and constructs or retires modes.
- [Diagnostics](Application/Diagnostics/Diagnostics.h) owns buffering and reports
  at outer-loop boundaries.
- [Snapshots](Snapshots/README.md) owns cross-concern state, association and history.

Input publishes interpreted intent. Session alone accepts normal mode switches,
reloads and exit decisions. The loop closes the old logical iteration before it
commits a replacement. Every activation receives a new generation and begins with
its own input publication.

One visual iteration consumes a frozen logical association plus immutable window
and graphics-output facts. Display prepares client-visible state. A mode describes
render content, Graphics executes it against the selected target, and Present
consumes that exact owning result. Resize or target replacement never retargets an
already rendered frame. Headless applications construct no Graphics instance and
skip Display, Render and Present.

The scheduler remains serial and blocking. Its contracts prepare independent
logical and visual scheduling but do not introduce threads, pacing or a backend.
Concrete mode bodies, adapters, simulation execution/extraction and diagnostic
recording remain documented outlines. Production execution is not redirected yet.
