/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {
class ILoadingServices;
struct LoadingFrameResult {
	bool frameDrawn;
	bool presentedInternally;
};

/**
 * Maintain the lobby and render a loading screen on its established context.
 * The runtime supplies ordinary loading synchronization; synchronous callbacks
 * already execute within the loading operation and progress-lock scope.
 * No controller pointer or graphics state is stored in this visual companion.
 */
class LoadingVisuals {
public:
	explicit LoadingVisuals(ILoadingServices& services);
	LoadingFrameResult RenderFrame();

private:
	void PrepareLoadingFrame();
	void RenderLoadingIntro();
	ILoadingServices& services;
};

//FIXME [LOAD-006] CLoadScreen::Draw internally swaps every ST draw, whereas the
// normal runtime also swaps after a successful Draw. SetLoadMessage has no outer
// swap. Preserve that observed distinction while tracing reachable ordinary ST
// iterations; do not infer that all ST draws are progress draws. This result
// exposes the internal present but the shared visual contract cannot yet carry
// it. Define ordinary/progress scheduling and assert exact swap counts before
// activation; attaching a generic presenter could add a second progress swap.
//
//FIXME [LOAD-007] LuaMenu::Update, LuaIntro::Update and draw callbacks occur in
// CLoadScreen::Draw under its current invocation context. Splitting preparation
// into an earlier unguarded phase changes thread/context and callback ordering.
// Keep this ordered companion until fake traces and hardware results justify
// the split. Audit callback-triggered replacement/intro deletion before deciding
// whether to recheck eligibility or retain the original single intro check.
}
