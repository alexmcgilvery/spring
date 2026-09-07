# Runtime architecture and concern skeletons

SystemNext now retains executable **orchestration and interface structure**, while
mode-specific engine work remains documented outlines. Removing implementation
must not remove the architecture that connects those outlines.

Start with [ApplicationLoop.cpp](ApplicationLoop.cpp): `Run` surrounds iteration with
host lifecycle, and `Update` visibly performs:

```text
input → session → display → render → present
           └─ authoritative simulation inside Game session
```

All five modes extend [IMode](Modes/IMode.h). Its immutable capabilities declare
session presence and display placement independently. There is no whole-controller
Update/Draw dispatcher. Absent optional concerns use the base scheduling-error
implementation; modes do not invent dummy work.

## Explicit contexts through a common interface

The [shared concern context types](Modes/ModeContexts.h) are variants of concrete
mode-local bundles. Each call carries one alternative, not all engine globals.
Concrete mode methods bind their own alternative with `std::get`, then retain the
expected-work documentation. A wrong alternative throws a programming error.
Existing [Game contexts](Modes/Game/GameContext.h) still distinguish mutable session
work, client preparation, live world views and prepared-frame input/output.

[ModeContextProvider](Modes/ModeContextProvider.h) is the application-side construction
contract. It creates fresh bundles from invocation metadata and owns temporary
preparation storage per iteration/activation. It cannot dispatch callbacks or retire
modes during construction. Its engine implementation does not exist yet.

[Mode binding](Modes/ModeBinding.h) borrows `IMode*`; null means inactive. The host
must advance generation for every activation, including same-address reuse. The
loop never repeats input/session for a replacement, and it rejects obsolete visual
work after display/render changes the binding. A mode needing before-graphics
display, but first selected during graphics acquisition, waits until the next
iteration instead of rendering unprepared state.

## Shared execution boundaries

[ApplicationHost](Globals/Lifecycle/ApplicationHost.h) provides lifecycle, platform
input, invocation sampling, graphics acquisition, presentation and diagnostic
boundaries. It has no engine implementation. Input collection must not also call
the mode's Input concern. The graphics scope spans dependent display/render/present;
iteration contexts retire before graphics unlock, including exception unwinding.
Diagnostic flushing follows normal iteration scope cleanup.

Exceptions propagate to the caller. Emergency shutdown remains the outer owner's
responsibility; normal shutdown follows the loop. This is explicit orchestration,
not an implemented legacy lifecycle adapter.

## What remains outlined

Mode bodies perform only typed context binding; simulation, publication and shared
resource behavior remain outlines. Frame eligibility/outcomes, context construction,
loading-progress execution and production entry-point redirection still require
implementation. Ordinary present currently follows a normally returned Render with
a stable binding; no legacy draw-result adaptation is claimed.

The engine does not register or invoke this architecture yet. SystemNext replacing
the active loop remains the objective; empty concerns must not be treated as an
operational engine. Legacy annotations and implementation adaptation remain next work.
No legacy source changed to support this correction.

## Verification

```sh
python3 -B test/engine/SystemNext/check_skeletons.py --compile-dir /tmp/systemnext-architecture-check
```

Headers and source units compile/link independently with legacy/headless definitions.
Compile-time checks verify interface inheritance and typed access. The standalone
fake-host executable checks actual loop ordering, capabilities, context construction,
transitions, graphics scope lifetime and exception behavior. It does not execute
legacy game code or establish gameplay compatibility.

Historical implementations are preserved in
`rfc0-artifacts/documented-skeleton/before-reset.tar.gz` at the workspace root.
