/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LifecycleAdapter.h"

#include "../Modes/Game/GameMode.h"

#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "System/SpringApp.h"
#include "System/Log/ILog.h"

namespace runtime {

LifecycleAdapter::LifecycleAdapter(SpringApp& app)
	: app(app)
{
	LOG("[runtime::LifecycleAdapter] constructed");
}

LifecycleAdapter::~LifecycleAdapter()
{
	LOG("[runtime::LifecycleAdapter] destroyed");
}

void LifecycleAdapter::Initialize()
{
	LOG("[runtime::LifecycleAdapter] Initialize");
	// Delegate to SpringApp::Init() which creates the window, GL context,
	// fonts, filesystem, global structures and the initial controller.
	if (!app.Init()) {
		LOG_L(L_ERROR, "[runtime::LifecycleAdapter] SpringApp::Init() failed");
		throw std::runtime_error("SpringApp::Init() failed");
	}
}

std::shared_ptr<IMode> LifecycleAdapter::CreateInitialMode()
{
	LOG("[runtime::LifecycleAdapter] CreateInitialMode");
	// SpringApp::Init() -> Startup() already set activeController.
	// Return a placeholder GameMode; the actual controller dispatch
	// happens in PlatformAdapter::BeginIteration().
	return std::make_shared<GameMode>();
}

std::shared_ptr<IMode> LifecycleAdapter::CreateMode(const LifecycleRequest& request)
{
	LOG("[runtime::LifecycleAdapter] CreateMode action=%d", static_cast<int>(request.action));

	if (request.action == LifecycleAction::Reload) {
		// Delegate to SpringApp::Reload() with the correct reload script.
		// gameSetup->reloadScript is copied because ResetState clears it.
		app.Reload(gameSetup->reloadScript);
		return std::make_shared<GameMode>();
	}

	if (request.action == LifecycleAction::SwitchMode) {
		return std::make_shared<GameMode>();
	}

	return nullptr;
}

void LifecycleAdapter::Shutdown()
{
	LOG("[runtime::LifecycleAdapter] Shutdown");
	// Delegate to SpringApp::Kill(true) which tears down all engine resources.
	// The killedCount guard prevents double-teardown if the forced Kill(false)
	// path from errorhandler.cpp also executes.
	SpringApp::Kill(true);
}

} // namespace runtime
