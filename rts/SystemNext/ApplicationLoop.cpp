/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ApplicationLoop.h"
#include "Globals/Lifecycle/ApplicationHost.h"
#include "Modes/IMode.h"
#include "Modes/ModeBinding.h"
#include "Modes/ModeContextProvider.h"
#include <stdexcept>
namespace runtime {
namespace {
struct ContextLifetime {
	ModeContextProvider& contexts;
	~ContextLifetime() { contexts.EndIteration(); }
};
}
void ApplicationLoop::Run(const ApplicationContext& context)
{
	/*
	 * Expected legacy sources (investigation starting points):
	 * [SpringApp.cpp](../System/SpringApp.cpp) — SpringApp::Run(), Update(),
	 * MainEventHandler(), Init(), Reload(), Kill()
	 *
	 * Context contract:
	 * [ApplicationContext.h](ApplicationContext.h) — ApplicationContext borrows
	 * host-lifetime input, configuration, mode selection, lifecycle, graphics and
	 * diagnostics resources. It is not the context passed wholesale into modes.
	 * The application constructs a fresh, narrow concern context after mode
	 * selection and again whenever that activation changes. Concrete context assembly is not implemented; the loop dispatches
	 * through contracts and performs no global lookups.
	 *
	 * Expected responsibility:
	 * Own application iteration and the visible ordering of mode concerns, while lifecycle
	 * establishes and retires their dependencies.
	 *
	 * Expected work, in conceptual order:
	 * Initialize the application; repeat input, session, display, render and shared present;
	 * service reload or exit requests; complete shutdown. Advance simulation only within
	 * accepted session work.
	 *
	 * Expected dependencies:
	 * An active mode and its lifetime, platform events, session readiness, real-time
	 * measurements, graphics availability and lifecycle requests. The context declares host dependencies; context construction and
	 * resource ownership still require implementation.
	 *
	 * Expected relationships:
	 * Mode blocks define behavior. This loop determines when they execute. Mode selection may
	 * change during input, session work or loading; subsequent blocks must describe which mode
	 * they belong to.
	 */

	/*
	 * Input and lifecycle boundary:
	 * Expect platform filtering and routing before ordinary mode work. Account for watchdog
	 * service, queued saves, window/configuration changes and reload decisions. The later
	 * annotation pass must identify their actual legacy ordering.
	 */

	/*
	 * Session and authoritative simulation:
	 * Expect session-bearing modes to process their own work. Game session processing may
	 * advance zero, one or multiple authoritative frames. There is no loop-local simulation
	 * accumulator or replacement command queue.
	 */

	/*
	 * Display, render and present:
	 * Expect the selected mode to update client presentation, prepare visual state and draw,
	 * followed by shared window presentation. Loading can request progress-driven execution
	 * while normal iteration is occupied; its relationship to this order needs a separate
	 * annotated path.
	 */

	/*
	 * Transitions and eventual scheduling separation:
	 * Expect newly selected modes to be considered before later concerns without repeating
	 * completed work. Account for null modes, cancellation, exceptions and teardown. Rendering
	 * and present initially block session work; independent scheduling later requires valid
	 * owning inputs and explicit synchronization, not merely a new thread.
	 */
	// [ lifecycle ] Establish resources before iterating; normal shutdown follows.
	auto& host = context.host;
	host.Initialize();
	std::uint64_t iteration = 0;
	while (host.BeginIteration()) {
		Update(context, ++iteration);
		// [ diagnostics ] Frame data and graphics scopes have already retired.
		host.FlushDiagnostics();
	}
	host.Shutdown();
}

/**
 * One visible pass through mode concerns. Rebind after input/session and graphics
 * synchronization. A replacement receives no second input or session call. A mode
 * replaced during display/render cannot present its obsolete frame; continue with
 * its replacement next iteration. No independent simulation pacing is introduced.
 */
void ApplicationLoop::Update(const ApplicationContext& context, std::uint64_t iteration)
{
	auto& [host, contexts, activeMode] = context;

	// [ input ] Platform collection precedes one selected mode's interpretation.
	host.CollectInput();
	if (host.ReloadRequested()) {
		host.Reload();
		return;
	}
	std::unique_ptr<GraphicsScope> graphics;
	contexts.BeginIteration(iteration);
	ContextLifetime contextLifetime{contexts};
	auto selected = activeMode;
	if (selected.mode == nullptr)
		return;
	{
		const auto invocation = host.CaptureInvocation({selected.mode->kind, selected.generation}, iteration);
		selected.mode->Input(contexts.Input(invocation));
	}
	if (host.ReloadRequested())
		return; // Retire invocation storage before next iteration performs reload.

	// [ session / simulation ] Only the selected session owns authoritative ticks.
	selected = activeMode;
	if (selected.mode != nullptr && selected.mode->handlesSession) {
		const auto invocation = host.CaptureInvocation({selected.mode->kind, selected.generation}, iteration);
		selected.mode->Session(contexts.Session(invocation));
	}
	selected = activeMode;
	if (selected.mode == nullptr || host.ReloadRequested())
		return;

	// [ display ] Client-only maintenance can precede graphics acquisition.
	if (selected.mode->displayPhase == DisplayPhase::BeforeGraphics) {
		const auto invocation = host.CaptureInvocation({selected.mode->kind, selected.generation}, iteration);
		selected.mode->Display(contexts.Display(invocation));
		if (!(activeMode == selected))
			return;
	}

	// [ graphics ] Scope spans dependent display, rendering and presentation.
	graphics = host.AcquireGraphics();
	if (!graphics)
		throw std::logic_error("Host returned no graphics scope");
	const auto beforeGraphics = selected;
	selected = activeMode;
	if (selected.mode == nullptr)
		return;
	if (selected.mode->displayPhase == DisplayPhase::BeforeGraphics && !(selected == beforeGraphics))
		return; // Replacement needs its before-graphics display next iteration.

	// [ display ] Graphics-dependent preparation belongs inside the scope.
	if (selected.mode->displayPhase == DisplayPhase::WithGraphics) {
		const auto invocation = host.CaptureInvocation({selected.mode->kind, selected.generation}, iteration);
		selected.mode->Display(contexts.Display(invocation));
	}
	if (!(activeMode == selected) || host.ReloadRequested())
		return;

	// [ render ] The provider supplies this activation's prepared frame context.
	{
		const auto invocation = host.CaptureInvocation({selected.mode->kind, selected.generation}, iteration);
		selected.mode->Render(contexts.Render(invocation));
	}
	if (!(activeMode == selected) || host.ReloadRequested())
		return;

	// [ present ] Serial and blocking. Loading progress requires a separate path.
	host.Present(PresentationOrigin::OrdinaryIteration);
}
}
