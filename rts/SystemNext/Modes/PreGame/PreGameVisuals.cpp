/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PreGameVisuals.h"

#include "IPreGameServices.h"

namespace runtime {
/**
 * Keep the connection screen's text batch and original read order together.
 *
 * No draw counter, idle delay, client update or internal swap exists in this
 * path. Headless execution retains the original successful draw result while
 * skipping the complete graphics body, rather than calling empty GL adapters.
 */
bool PreGameVisuals::PrepareAndRender()
{
	auto& services = ConnectionServices();

	if (!services.HasConnectionScreen())
		return true;

	services.ClearConnectionScreen();
	services.BeginConnectionText();
	services.DrawConnectionStatus();
	services.DrawConnectionIdentity();
	services.DrawArchiveChecksumProgress();
	services.DrawAbortInstructionsAndCredits();
	services.EndConnectionText();
	return true;
}
}
