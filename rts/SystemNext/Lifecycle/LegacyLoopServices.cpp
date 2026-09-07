/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyLoopServices.h"

#include <utility>
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Input/InputHandler.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/Platform/Watchdog.h"
#include "System/SpringApp.h"
#include "SystemNext/Diagnostics/LegacyRuntimeDiagnostics.h"

namespace runtime::legacy {
LegacyLoopServices::LegacyLoopServices(SpringApp& host): host(host)
{}

LoopServices LegacyLoopServices::Bind()
{
	return {.input = *this, .lifecycle = *this, .platform = *this,
		.diagnostics = *this, .mode = mode, .session = session, .visuals = visuals};
}

void LegacyLoopServices::ProcessEvents()
{
	input.PushEvents();
}

bool LegacyLoopServices::ExitRequested() const
{
	return gu->globalQuit;
}

bool LegacyLoopServices::ReloadRequested() const
{
	return gu->globalReload;
}

void LegacyLoopServices::ServiceWatchdog()
{
	Watchdog::ClearTimer(WDT_MAIN);
}

void LegacyLoopServices::ProcessQueuedSave()
{
	ILoadSaveHandler::CreateSave(std::move(globalSaveFileData));
}

/** Reload takes a value because resetting the session clears its setup script. */
void LegacyLoopServices::ReloadSession()
{
	host.Reload(gameSetup->reloadScript);
}

void LegacyLoopServices::RequestExit()
{
	gu->globalQuit = true;
}

void LegacyLoopServices::UpdateConfiguration()
{
	configHandler->Update();
}

void LegacyLoopServices::UpdateWindow()
{
	globalRendering->UpdateWindow();
}

void LegacyLoopServices::UpdateClock()
{
	globalRendering->UpdateTimer();
}

PhaseToken LegacyLoopServices::BeginPhase(Phase phase) noexcept
{
	return legacy::BeginPhase(phase);
}

void LegacyLoopServices::EndPhase(PhaseToken token) noexcept
{
	legacy::EndPhase(token);
}

void LegacyLoopServices::Flush() noexcept
{
	DrainDiagnostics();
}
}
