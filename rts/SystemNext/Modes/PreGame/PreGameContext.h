/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Globals/InvocationContext.h"
#include <span>
union SDL_Event;
class CPreGame;
class ClientSetup;
class CGameSetup;
class CglFont;
namespace netcode { class CNetProtocol; }
namespace runtime {
struct GraphicsAccess;
struct LifecycleRequests;
struct PreGameInputContext {
	const InvocationContext& invocation;
	std::span<const SDL_Event> events;
	CPreGame& connection;
	LifecycleRequests& requests;
};
struct PreGameSessionContext {
	const InvocationContext& invocation;
	CPreGame& connection;
	ClientSetup& setup;
	netcode::CNetProtocol& network;
	LifecycleRequests& requests;
};
struct PreGameRenderContext {
	const InvocationContext& invocation;
	const ClientSetup& setup;
	const netcode::CNetProtocol& network;
	CglFont& font;
	GraphicsAccess& graphics;
};
/* The render view borrows live setup/network state; it is not a frozen snapshot.
 * Task/save-handler ownership stays unresolved inside the connection adapter.
 * These references neither reveal private fields nor make worker access safe. */
}
