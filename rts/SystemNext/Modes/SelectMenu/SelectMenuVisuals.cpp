/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectMenuVisuals.h"

#include "ISelectMenuServices.h"

namespace runtime {
SelectMenuVisuals::~SelectMenuVisuals() = default;

/**
 * Pace an otherwise idle menu before accounting for and drawing its frame.
 * GUI cleanup remains inside interface drawing, after screen preparation,
 * because it may destroy the departing menu and its callback-owned windows.
 * No work after that boundary relies on a menu object surviving the draw.
 */
bool SelectMenuVisuals::PrepareAndRender()
{
	auto& services = GetServices();

	services.WaitForMenuFrame();
	services.AdvanceDrawCounter();
	services.ClearMenuScreen();
	services.DrawMenuInterface();

	return true;
}
}
