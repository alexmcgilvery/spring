/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacySession.h"

#include "Game/Game.h"
#include "Game/GameController.h"
#include "Game/PreGame.h"
#include "Game/LoadScreen.h"
#include "SystemNext/Diagnostics/LegacyRuntimeDiagnostics.h"

namespace runtime::legacy {
bool UpdateCurrentController()
{
	const auto* before = activeController;
	const bool continueRunning = activeController == nullptr || activeController->Update();
	ObserveController(before, activeController);
	return continueRunning;
}

SessionUpdate LegacySession::Advance()
{
	return SessionUpdate::FromContinuation(UpdateCurrentController());
}

/**
 * Game startup, loading and gameplay advance session lifetime or authority.
 * Menus have no session and perform their existing update as client work.
 * Compare only currently registered objects; an object can replace itself
 * during update, so no controller address is retained by this adapter.
 */
bool LegacyRuntimeMode::HandlesSession() const
{
	return activeController != nullptr && (
		activeController == game || activeController == pregame || activeController == CLoadScreen::GetInstance()
	);
}

SessionUpdate LegacyRuntimeMode::UpdateSession(Session& session)
{
	return session.Advance();
}
}
