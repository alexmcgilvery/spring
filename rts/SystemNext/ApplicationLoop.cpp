/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ApplicationLoop.h"

#include "Diagnostics/LoopPhaseScope.h"
#include "Presentation/IVisualFrame.h"
#include "Session/IRuntimeMode.h"

namespace runtime {
ApplicationLoop::ApplicationLoop(LoopServices services):
	input(services.input), lifecycle(services.lifecycle), platform(services.platform),
	diagnostics(services.diagnostics), activeMode(services.mode), session(services.session), visuals(services.visuals)
{}

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
	ProcessInputAndLifecycle();

	if (lifecycle.ReloadRequested())
		ReloadSession();
	else
		UpdateAndDraw();

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
	const auto [applicationStatus, visualContext] = ServiceSession();
	const auto clientStatus = visuals.ExecuteFrame(visualContext);
	ApplyApplicationStatus(applicationStatus, clientStatus);
}

/**
 * Keep authoritative session service outside rendering and display waits.
 * A mode without a session leaves its interaction update to client presentation.
 * Session service owns command/tick ordering; this loop has no tick accumulator.
 */
SessionUpdate ApplicationLoop::ServiceSession()
{
	if (!activeMode.HandlesSession())
		return SessionUpdate::NoSession();
	return activeMode.UpdateSession(session);
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
