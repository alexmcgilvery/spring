/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Globals/InvocationContext.h"
#include <span>
union SDL_Event;
class SelectMenu;
class ClientSetup;
class ConfigHandler;
namespace agui { class Gui; }
namespace runtime {
struct GraphicsAccess;
struct LifecycleRequests;
struct SelectMenuInputContext {
	const InvocationContext& invocation;
	std::span<const SDL_Event> events;
	SelectMenu& menu;
	ClientSetup& setup;
	ConfigHandler& configuration;
	agui::Gui& gui;
	LifecycleRequests& requests;
};

struct SelectMenuRenderContext {
	const InvocationContext& invocation;
	GraphicsAccess& graphics;
	agui::Gui& gui;
};
/* Events are an ordered borrow of the batch selected by platform routing, not
 * another queue. GUI/setup mutation is input-local; rendering receives no setup
 * or network authority. Menu references do not grant access to private callbacks. */
}
