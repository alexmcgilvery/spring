/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyVisualFrame.h"

#include "Game/GameController.h"
#include "Present.h"
#include "SystemNext/Diagnostics/LegacyRuntimeDiagnostics.h"

namespace runtime::legacy {
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

/** False still reaches the swap implementation, which may force presentation. */
void LegacyVisualFrame::Present(bool allowSwap)
{
	runtime::PresentWindow(allowSwap);
}
}
