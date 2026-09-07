/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LoadingMode.h"

#include "ILoadingServices.h"
#include "LoadingVisuals.h"

namespace runtime {
bool LoadingMode::HandlesSession() const
{
	return true;
}

SessionUpdate LoadingMode::UpdateSession(Session&)
{
	UpdateLoading();
	// Completion replaces the mode; it is not an application exit request.
	return SessionUpdate::FromContinuation(true);
}

LoadingUpdate LoadingMode::UpdateLoading()
{
	// [ progress ] Deliver notifications at the existing pre-completion point.
	ResolveLoadingServices().DeliverProgressNotifications();

	// [ completion ] Retirement ends all access to loading-controller storage.
	if (ResolveLoadingServices().IsGameLoadingComplete()) {
		ResolveLoadingServices().RetireLoadingController();
		ResolveLoadingServices().AnnounceLoadingCompletion();
		return LoadingUpdate::ControllerRetired;
	}

	// [ responsiveness ] Synchronous loading must service window/watchdog work.
	if (!ResolveLoadingServices().IsMultithreadedLoading())
		ResolveLoadingServices().KeepWindowResponsive();

	return LoadingUpdate::Pending;
}

void LoadingMode::ReportProgress(std::string_view text, bool replaceLast, LoadingVisuals& visuals)
{
	// [ input ] Loading must pump window events/watchdog before locking.
	// [ display ] Queue the progress text, preserving logging and FPU checks.
	ResolveLoadingServices().KeepWindowResponsive();
	auto progressScope = ResolveLoadingServices().QueueProgressNotification(text, replaceLast);
	if (ResolveLoadingServices().IsMultithreadedLoading())
		return;

	// [ synchronous progress ] Loading drives these frames on the caller's
	// context. There is no ordinary runtime present at the end of this entry.
//FIXME [LOAD-005] The source-equivalent flow has an unconditional draw after
	// Update, even if completion retired CLoadScreen. Keep those exact operations
	// visible but disabled beside the proposed retirement guard. Prove completion
	// reachability and mutex lifetime before selecting either path for activation.
#if 0
	UpdateLoading();
	visuals.RenderFrame();
#else
	if (UpdateLoading() == LoadingUpdate::ControllerRetired)
		return;

	// [ display / render / present ] Same loading operations as an ordinary
	// visual frame; the caller does not add an ordinary window present here.
	visuals.RenderFrame();
#endif
}

//FIXME [LOAD-005] ReportProgress deliberately stops after retirement; the old
// SetLoadMessage path unconditionally calls Draw after Update. This is a proposed
// safety boundary, not demonstrated behavior parity. Prove whether completion
// is reachable within Game::Load progress callbacks and resolve LOAD-001's held
// lock before enabling this path. A mode surviving its controller is insufficient
// if the progress scope still refers to the controller's destroyed mutex.
}
