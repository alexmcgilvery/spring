/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyVisualFrame.h"

#include "Game/GameController.h"
#include "SystemNext/Session/LegacySession.h"
#include "SystemNext/Diagnostics/LegacyRuntimeDiagnostics.h"

namespace runtime::legacy {
/**
 * Update menus before loading synchronization, just as session updates run.
 * This does not run game presentation preparation: that remains nested in Draw.
 */
ApplicationStatus LegacyVisualFrame::UpdateClientMode()
{
	return UpdateCurrentController() ? ApplicationStatus::Continue : ApplicationStatus::ExitRequested;
}

/** The loading guard may be a no-op; acquire does not always bind a GL context. */
void LegacyVisualFrame::LockDraw()
{
	context = CLoadLock::GetThreadSafety();
	lock.emplace(CLoadLock::GetUniqueLock());
	ObserveGuard(true, context);
}

void LegacyVisualFrame::UnlockDraw() noexcept
{
	lock.reset();
	ObserveGuard(false, context);
}

/**
 * Resolve the active controller after update and guard acquisition.
 * Its Draw still combines presentation preparation, world/Lua/UI rendering
 * and capture. An extra unsynced update here would execute that work twice.
 */
bool LegacyVisualFrame::Draw()
{
	return activeController != nullptr && activeController->Draw();
}

/** False still reaches the swap implementation, which may force presentation. */
void LegacyVisualFrame::Present(bool allowSwap)
{
	PhaseScope phase(Phase::Swap);
	globalRendering->SwapBuffers(allowSwap, false);
}
}
