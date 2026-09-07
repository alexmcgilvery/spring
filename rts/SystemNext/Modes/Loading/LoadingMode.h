/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>

#include "SystemNext/Modes/Mode.h"

class CLoadScreen;
class ILoadSaveHandler;

namespace runtime {
/**
 * Run loading with one existing owner for queued progress, threads and timing.
 * Session completion precedes graphics work. Display/render share frame-owned
 * eligibility and profiling state; presenting belongs to their caller. Progress
 * callbacks have their own synchronous execution path because Game::Load can
 * occupy the main stack before ordinary application iterations resume.
 */
//FIXME ADAPTER-LOADING-BACKING: The implementation copy currently
// reads private legacy fields/helpers. Its added friendship and legacy-side
// forwarding have been removed. Supply backing state/access entirely in
// SystemNext before compiling or connecting this adapter; legacy stays intact.
class LoadingMode final : public Mode {
public:
	bool HandlesSession() const final;
	DisplayPhase GetDisplayPhase() const final;
	SessionUpdate UpdateSession(Session& session) final;
	ApplicationStatus UpdateDisplay(ModeFrame& frame) final;
	RenderResult Render(ModeFrame& frame) final;

	SessionUpdate ReportProgress(CLoadScreen& controller, const std::string& text, bool replaceLast, Session& session);
	void BeginLoading(std::string&& mapFileName, std::string&& modFileName, ILoadSaveHandler* saveFile);
	bool InitializeLoading();
	void StopLoadingResources();
	void FinishControllerDestruction();
	void ResizeEvent() final;
	int KeyPressed(int keyCode, int scanCode, bool isRepeat) final;
	int KeyReleased(int keyCode, int scanCode) final;

private:
	void RetireLoadingController();
	void AnnounceLoadingCompletion();
};
}
