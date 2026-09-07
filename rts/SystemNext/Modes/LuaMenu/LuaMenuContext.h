/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Globals/InvocationContext.h"
#include <span>
union SDL_Event;
class CLuaMenu;
class CLuaMenuController;
class CEventHandler;
class CMouseHandler;
class CInfoConsole;
namespace runtime {
struct GraphicsAccess;
struct LifecycleRequests;
struct LuaMenuInputContext {
	const InvocationContext& invocation;
	std::span<const SDL_Event> events;
	CLuaMenuController& menu;
	CEventHandler& eventsSink;
	LifecycleRequests& requests;
};
struct LuaMenuDisplayContext {
	const InvocationContext& invocation;
	CLuaMenu& scripts;
	CEventHandler& events;
	CMouseHandler& mouse;
	CInfoConsole& console;
};
struct LuaMenuRenderContext {
	const InvocationContext& invocation;
	CLuaMenuController& menu;
	CLuaMenu& scripts;
	CMouseHandler& mouse;
	GraphicsAccess& graphics;
};
/* Required references mean the caller must resolve a valid menu before invoking
 * the concern. Script callbacks may transition modes; no references survive that
 * retirement. Render keeps mutable menu access for frame timing, not session work. */
}
