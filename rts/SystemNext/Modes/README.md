# Owned modes

Each directory contains one mode's researched orchestration and visual companion:
SelectMenu, LuaMenu, PreGame, Loading and Game. An absent controller is an inactive
binding, not another mode. Replay, hosting, joining and save loading are paths
within these five modes.

Mode.cpp owns session or client-maintenance blocks. Visuals.cpp owns preparation
and rendering blocks. I*Services.h declares the required backing operations without
engine dependencies. Concrete Legacy*Services adapters will eventually extract and
reuse the existing operations while retaining their current state owners.

These block-outs are unbound and excluded from production sources and dispatch.
Abstract service operations are requirements, not completed extraction. Some mode
entry points remain abstract where activation or capability contracts are unresolved.
The running application still uses its existing compatibility path.

Read source briefs for ownership, ordering, timing and failure constraints. Search
`//FIXME` for unresolved concerns and their required evidence. No proposed operation
or comment establishes that a lifetime hazard is reproduced or that a migration is
validated. Binding must resolve those concerns and preserve one implementation of
each extracted operation; forwarding the entire controller update/draw is not an
extraction.

The shared VisualFrameContext contract also records unresolved capability/completion
and retirement boundaries. Mode-specific research does not authorize a different
application loop, an additional input route, or independent graphics execution.

Migration order is SelectMenu -> LuaMenu -> PreGame -> Loading -> Game. Validate
with substitute adjacent modes before real-mode fixtures. Game has separate session,
presentation/preparation and rendering/capture milestones. Compilation and source
checks establish structural validity only; behavioral and graphics evidence remains
necessary for activation.

## Read the complete extraction drafts

The Legacy* files now retain the actual backing statements and branches, grouped
under input, session, display, render and present responsibilities. Follow each
Mode/Visuals operation to its proposed backing body. A pure interface declaration
or prose summary is not a substitute for that code.

Conflicting code stays at its intended boundary with a local `//FIXME`. These
candidates are explicitly disabled with `#if 0` while access, lifetime or scope
contracts are unresolved. Their bodies are reviewable code, not type-checked or
activated adapters. The existing controller remains the sole executable owner of
those statements. Remove the old implementation when a reviewed extraction is
activated; do not enable a second production implementation.

- SelectMenu: LegacySelectMenuInput retains selection/settings/cancellation and
  GUI activation; LegacySelectMenuServices supplies frame pacing and rendering.
- LuaMenu: LegacyLuaMenuServices retains event input, display maintenance,
  rendering, reset and activation, including the synchronous Lua boundaries.
- PreGame: LegacyPreGameServices retains setup/session, cancellation and
  connection display/render bodies, including worker and retirement conflicts.
- Loading: LegacyLoadingServices retains startup, progress, responsiveness,
  display/render/internal present and teardown bodies.
- Game: LegacyGameServices retains session service; LegacyGameVisualServices
  retains display/input/render/capture bodies and their original timing branches.

Moving a statement to a different concern can change its invocation order, scope,
thread or backing lifetime. The source FIXME must describe that specific conflict
beside the retained statement. For example, command submission still appears in
Game's display draft because moving it to the earlier input block requires an
explicit ordering decision; it has not silently disappeared behind an interface.
