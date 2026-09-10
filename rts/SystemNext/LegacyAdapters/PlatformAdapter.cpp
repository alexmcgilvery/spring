/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PlatformAdapter.h"

#include "Game/GlobalUnsynced.h"
#include "Game/GameController.h"
#include "Game/GameSetup.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Input/InputHandler.h"
#include "System/Platform/Threading.h"
#include "System/LoadLock.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/Watchdog.h"

namespace runtime {

PlatformAdapter::PlatformAdapter()
{
	LOG("[runtime::PlatformAdapter] constructed");
}

PlatformAdapter::~PlatformAdapter()
{
	LOG("[runtime::PlatformAdapter] destroyed");
}

// CONCERN: input
// CONCERN: session
// CONCERN: render
// CONCERN: present
bool PlatformAdapter::BeginIteration()
{
	// Reset the main-thread watchdog so the watchdog thread doesn't kill the process.
	Watchdog::ClearTimer(WDT_MAIN);

	// Pump SDL events through the existing InputHandler. The registered handlers
	// (including SpringApp::MainEventHandler) dispatch to the active controller.
	input.PushEvents();

	// Move to clear global data if a save is queued.
	ILoadSaveHandler::CreateSave(std::move(globalSaveFileData));

	// Handle reload: let the ApplicationLoop's lifecycle path handle this.
	if (gu->globalReload)
		return true;

	// Do the full frame work that was previously in SpringApp::Update():
	// config update, window/timer update, controller Update + Draw, swap buffers.
	configHandler->Update();
	globalRendering->UpdateWindow();
	globalRendering->UpdateTimer();

	// sic; Update can set the controller to null
	const bool retc = (activeController == nullptr || activeController->Update());

	auto lock = CLoadLock::GetUniqueLock();
	const bool swap = (retc && activeController != nullptr && activeController->Draw());

	// always swap by default, not doing so can upset some drivers
	globalRendering->SwapBuffers(swap, false);

	return true;
}

PlatformPublications PlatformAdapter::CollectPublications()
{
	PlatformPublications publications;

	// Package the current window state as an immutable publication.
	publications.window.windowId = 1;
	publications.window.width = static_cast<unsigned>(globalRendering->viewSizeX);
	publications.window.height = static_cast<unsigned>(globalRendering->viewSizeY);
	publications.window.focused = globalRendering->active;
	publications.window.visible = globalRendering->active;

	publications.input.sampledAt = std::chrono::nanoseconds(0);
	publications.input.realDelta = std::chrono::nanoseconds(0);

	return publications;
}

bool PlatformAdapter::ExitRequested() const
{
	return (gu != nullptr && gu->globalQuit);
}

bool PlatformAdapter::ReloadRequested() const
{
	return (gu != nullptr && gu->globalReload);
}

} // namespace runtime
