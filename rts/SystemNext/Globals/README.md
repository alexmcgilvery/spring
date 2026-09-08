# Application-wide responsibilities

- [Snapshots](Snapshots/README.md) owns immutable publication storage, typed access,
  iteration association and admission. Mode-local schemas remain with modes.
- [Graphics](Graphics/VisualOutput.h) owns backend execution, resource synchronization,
  output leases and ordinary presentation. Headless execution has no instance.
- [Lifecycle](Lifecycle/ApplicationHost.h) supplies platform facts, prepares mode
  activation and owns application cleanup. It performs accepted Session decisions.
- [Diagnostics](Diagnostics/Diagnostics.cpp) outlines bounded observation and
  outer-boundary reporting. Concrete logging and analysis remain unimplemented.

These are identified shared responsibilities, not a general service locator.
Neither snapshot inputs nor the runtime mode interface expose mutable game globals.
Private resource contexts beneath simulation, lifecycle and diagnostics describe
their owner operations; they are not passed to ordinary mode concerns.
