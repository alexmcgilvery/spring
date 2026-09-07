# Shared runtime expectations

Only application-wide resources or responsibilities actually shared across modes
belong here. These are documentation skeletons, not service implementations.

- [Graphics](Graphics/Graphics.cpp): shared context/resources, synchronization and
  window presentation. Each mode keeps its drawing content. Loading's ordinary
  and progress-driven present routes need separate source annotation.
- [Lifecycle](Lifecycle/Lifecycle.cpp): application initialization, reload and
  shutdown. Mode-specific setup and transitions remain documented in modes.
- [Diagnostics](Diagnostics/Diagnostics.cpp): cross-mode execution observations
  and output expectations. Game's semantic simulation observation remains local.

There is no generic utility bucket. Any future addition must identify the shared
resource or cross-mode use that warrants placing it here.

Invocation metadata is shared because every concern uses the same activation/time
identity contract. Graphics access and lifecycle requests are shared resources;
mode-specific dependencies remain in the corresponding mode context. No universal
GlobalContext is passed to every mode.
