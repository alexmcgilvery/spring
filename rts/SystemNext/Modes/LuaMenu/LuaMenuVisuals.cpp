/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaMenuVisuals.h"

#include "ILuaMenuServices.h"

namespace runtime {
LuaMenuVisuals::LuaMenuVisuals(ILuaMenuServices& services): services(services)
{
}

/**
 * Permit scripts to skip ordinary drawing without suppressing the periodic
 * graphics refresh. Record completion only after the final callback returns.
 */
bool LuaMenuVisuals::PrepareAndRender()
{
	services.RequireActiveLuaHandler();
	if (!ShouldRender()) {
		services.SleepForSkippedDraw();
		return false;
	}

	PrepareFrame();
	RenderMenu();
	services.RecordCompletedDrawTime();
	return true;
}

/**
 * Avoid invoking AllowDraw for an inactive window. Still sample elapsed time
 * after permission, even when permission succeeds: this preserves callback and
 * clock order and the strict greater-than-30 whole-second forcing threshold.
 */
bool LuaMenuVisuals::ShouldRender()
{
	const bool allowDraw = services.IsWindowActive() && services.RequestScriptDrawPermission();
	const bool forceDraw = services.WholeSecondsSinceLastCompletedDraw() > 30;
	return allowDraw || forceDraw;
}

/**
 * Expose the new frame number to every rendering callback and establish the
 * same screen state before genesis callbacks can issue graphics operations.
 */
void LuaMenuVisuals::PrepareFrame()
{
	services.AdvanceDrawCounter();
	services.ClearScreen();
}

/**
 * Preserve EventHandler's graphics scopes and the cursor's position between
 * screen callbacks and post-screen overlays. Do not replace dispatch with a
 * direct LuaMenu call: subscriptions and drawing-state resets are observable.
 */
void LuaMenuVisuals::RenderMenu()
{
	services.DispatchDrawGenesis();
	services.DispatchDrawScreen();
	services.DrawCursor();
	services.DispatchDrawScreenPost();
}

//FIXME [LUA-TIMING] CLuaMenuController::Draw samples its first time after AllowDraw
// and stores a second time only after DrawScreenPost. The threshold uses toSecsi
// > 30, not a floating elapsed >= 30. Reuse the engine clock and original private
// timestamp when binding; test 30/31 whole seconds, inactive-window permission
// short-circuiting, exceptions before completion, reset/reactivation, and wrap of
// drawFrame. Moving completion to present would change forced refresh timing.

//FIXME [LUA-GRAPHICS] CEventHandler DRAW_CALLIN owns LuaOpenGL Enable/Reset/Disable
// around each callback list. ClearScreen/cursor drawing also touch live graphics
// state, and LuaMenu persists alongside a game. No immutable render input exists
// here: a worker cannot safely consume this companion until Lua, mouse, VFS and
// GL ownership are resolved. Validate the current main-thread/context contract,
// load-lock real/no-op variants, and exception unwinding through outer present.
}
