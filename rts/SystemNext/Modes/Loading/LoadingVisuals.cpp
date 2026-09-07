/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LoadingVisuals.h"

#include "ILoadingServices.h"

namespace runtime {
LoadingVisuals::LoadingVisuals(ILoadingServices& services)
	: services(services)
{
}

LoadingFrameResult LoadingVisuals::RenderFrame()
{
	// [ display ] Pacing, frame accounting and lobby maintenance precede intro eligibility.
	PrepareLoadingFrame();

	// [ render ] Intro update crosses from display into rendering here: genesis
	// runs before clear. Keep that boundary visible until LOAD-007 is resolved.
	RenderLoadingIntro();

	// [ present ] Synchronous loading presents within its caller's context.
	const bool presentInternally = !services.IsMultithreadedLoading();
	if (presentInternally)
		services.PresentSynchronousLoadingFrame();

	return {true, presentInternally};
}

void LoadingVisuals::PrepareLoadingFrame()
{
	services.PaceLoadingFrame();
	services.AdvanceDrawCounter();
	services.MaintainLobbyConnection();
}

void LoadingVisuals::RenderLoadingIntro()
{
	if (!services.HasLoadingIntro())
		return;

	services.UpdateLoadingIntro();
	services.DrawLoadingGenesis();
	services.ClearLoadingScreen();
	services.DrawLoadingScreen();
}
}
