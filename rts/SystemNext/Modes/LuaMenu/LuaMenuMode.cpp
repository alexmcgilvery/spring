/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaMenuMode.h"

#include "ILuaMenuServices.h"

namespace runtime {
LuaMenuMode::LuaMenuMode(ILuaMenuServices& services): services(services)
{
}

bool LuaMenuMode::HandlesSession() const
{
	return false;
}

/**
 * Make queued client changes visible before script interaction is evaluated.
 * The existing menu update returns continuation even if a callback requests
 * global quit/reload. The outer lifecycle boundary owns those requests.
 */
ApplicationStatus LuaMenuMode::UpdateClientState()
{
	services.RequireActiveLuaHandler();
	MaintainClientServices();
	UpdateMenuInteraction();
	return ApplicationStatus::Continue;
}

/**
 * Publish queued console notifications before menu Update callbacks so scripts
 * see the same messages and mouse/cursor state in the same iteration.
 */
void LuaMenuMode::MaintainClientServices()
{
	services.CollectScriptGarbage();
	services.PublishConsoleLines();
	services.UpdateMouse();
	services.UpdateCursors();
}

/**
 * Evaluate tooltip hit testing after the menu has updated its layout/state.
 * Calling the Lua handler directly would bypass other EventHandler clients.
 */
void LuaMenuMode::UpdateMenuInteraction()
{
	services.DispatchClientUpdate();
	services.EvaluateTooltip();
}

//FIXME [LUA-REENTRANCY] CLuaMenu::Enable/Disable queue destructive actions while
// IsRunning() is true to avoid freeing a VM inside pcall. CheckAction exists in
// LuaMenu.h, but the current source has no LuaMenu CheckAction invocation. Do not
// add a drain as an assumed missing step: it changes handler lifetime. Establish
// intended action delivery separately and test callback-triggered reload/disable,
// console notification nesting, tooltip callbacks, and EventHandler list removal.
// Service calls must preserve current completion order and resolve globals anew;
// arbitrary cancellation after each callback would also change current behavior.

//FIXME [LUA-TRANSITION] LuaUnsyncedCtrl::Reload/Restart set globalReload; Quit
// sets globalQuit; Start can replace the process (or return on failure). Client
// update returning Continue does not clear these requests. Prove input/update/
// tooltip-generated requests reach the existing outer boundary, and re-resolve
// the visual binding after update instead of drawing a retained mode/controller.
}
