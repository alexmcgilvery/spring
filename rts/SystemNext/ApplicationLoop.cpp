/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ApplicationLoop.h"

#include "Diagnostics/LoopPhaseScope.h"
#include "Presentation/IVisualFrame.h"
#include "Session/IRuntimeMode.h"
#include "Session/Session.h"
#include "Modes/Mode.h"
#include "Modes/ModeBinding.h"

namespace runtime {
ApplicationLoop::ApplicationLoop(LoopServices services):
	input(services.input), lifecycle(services.lifecycle), platform(services.platform),
	diagnostics(services.diagnostics), session(services.session), visuals(services.visuals), modes(services.modes)
{}

void ApplicationLoop::BlockCurrentMode(const BlockedFlow& failure)
{
	if (!failure.id.starts_with("MODE-CHANGED-"))
		blockedGeneration = modes.Generation();
	diagnostics.ReportBlocked(failure);
}

void ApplicationLoop::Run()
{
	while (!lifecycle.ExitRequested())
		RunIteration();
}

/**
 * Finish the selected iteration even if input requests exit partway through it.
 * Reload replaces normal update/draw work, while diagnostics drain on either
 * normal path. Engine exceptions unwind directly to the application handlers.
 */
void ApplicationLoop::RunIteration()
{
	LoopPhaseScope iteration(diagnostics, Phase::Host);
	try {
		ProcessInputAndLifecycle();
		if (lifecycle.ReloadRequested()) {
			blockedGeneration.reset();
			ReloadSession();
		} else {
			UpdateAndDraw();
		}
	} catch (const IncompleteFlow& failure) {
		BlockCurrentMode(failure.Failure());
	}

	FlushDiagnostics();
}

/**
 * Dispatch input and complete queued saves before selecting reload.
 * Input can request lifecycle changes. Saves require current session objects
 * to remain valid until serialization completes. Service the watchdog once
 * per iteration, including iterations spent on reload rather than gameplay.
 */
void ApplicationLoop::ProcessInputAndLifecycle()
{
	lifecycle.ServiceWatchdog();
	input.ProcessEvents();
	lifecycle.ProcessQueuedSave();
}

/** Reload replaces session dependencies, so no ordinary update follows it. */
void ApplicationLoop::ReloadSession()
{
	lifecycle.ReloadSession();
}

/** Apply configuration before window maintenance, then sample the frame clock. */
void ApplicationLoop::UpdatePlatformState()
{
	platform.UpdateConfiguration();
	platform.UpdateWindow();
	platform.UpdateClock();
}

/**
 * Preserve serial execution while visual work can still access live state.
 * Apply exit requests only after the guarded visual completion path returns.
 * Moving rendering off-thread without removing that dependency is insufficient.
 */
void ApplicationLoop::UpdateAndDraw()
{
	LoopPhaseScope update(diagnostics, Phase::Update);
	UpdatePlatformState();
	if (blockedGeneration && *blockedGeneration == modes.Generation())
		return;
	blockedGeneration.reset();
	UpdateModeBlocks();
}

/**
 * Select session/client update once; graphics rebinds after synchronization.
 * Input was already dispatched at the iteration boundary. Display completion
 * is explicit and independent of session capability. A replacement may render
 * this iteration, but must not receive a second before-graphics client update.
 */
void ApplicationLoop::UpdateModeBlocks()
{
	const ModeSelection update = modes.Select();

	// [ session / simulation ] Accepted stream processing owns simulation ticks.
	const auto result = session.AdvanceMode(update.mode);
	if (result.applicationStatus == ApplicationStatus::Blocked)
		throw IncompleteFlow(result.blocked.id, result.blocked.reason);
	ModeFrame frame;
	frame.allowRender = result.applicationStatus != ApplicationStatus::ExitRequested;
	frame.bindingGeneration = update.generation;

	// [ display ] Select client maintenance once, independently of session work.
	auto displayStatus = ApplicationStatus::Continue;
	if (frame.allowRender && update.mode != nullptr && modes.Select() == update &&
		update.mode->GetDisplayPhase() == DisplayPhase::BeforeGraphics) {
		displayStatus = update.mode->UpdateDisplay(frame);
		if (displayStatus == ApplicationStatus::Blocked)
			throw IncompleteFlow(frame.blocked.id, frame.blocked.reason);
		frame.allowRender &= displayStatus != ApplicationStatus::ExitRequested;
	}

	// [ display / render / present ] One shared graphics synchronization scope.
	const auto visualStatus = visuals.ExecuteModeFrame(modes, frame);
	ApplyApplicationStatus(result.applicationStatus,
		displayStatus == ApplicationStatus::ExitRequested ? displayStatus : visualStatus);
}

/** Requests are monotonic: an earlier exit request must never be cleared. */
void ApplicationLoop::ApplyApplicationStatus(ApplicationStatus sessionStatus, ApplicationStatus clientStatus)
{
	if (sessionStatus == ApplicationStatus::ExitRequested || clientStatus == ApplicationStatus::ExitRequested)
		lifecycle.RequestExit();
}

/** File serialization may block, so it stays outside hot execution callbacks. */
void ApplicationLoop::FlushDiagnostics()
{
	diagnostics.Flush();
}
}
