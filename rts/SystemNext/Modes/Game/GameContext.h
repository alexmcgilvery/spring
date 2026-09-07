/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Globals/InvocationContext.h"
#include "Simulation/Publication/PublishedFrame.h"
#include <span>
union SDL_Event;
class CGame;
class CGlobalSynced;
class CGlobalUnsynced;
class CCamera;
class CMouseHandler;
class CEventHandler;
class CInfoConsole;
class ISound;
class IVideoCapturing;
class CUnitHandler;
class CFeatureHandler;
class CProjectileHandler;
class JobDispatcher;
namespace netcode { class CNetProtocol; }
namespace runtime {
struct GraphicsAccess;
struct LifecycleRequests;
struct GameFrame;
/** Read-only live view, not a snapshot or a recursively immutable world. */
struct GameWorldView {
	const CGlobalSynced& simulation;
	const CUnitHandler& units;
	const CFeatureHandler& features;
	const CProjectileHandler& projectiles;
};
struct GameInputContext {
	const InvocationContext& invocation;
	std::span<const SDL_Event> events;
	CGame& game;
	CMouseHandler& mouse;
	CEventHandler& eventSink;
	netcode::CNetProtocol& commands;
	LifecycleRequests& requests;
};
struct GameSessionContext {
	const InvocationContext& invocation;
	CGame& game;
	netcode::CNetProtocol& network;
	JobDispatcher& jobs;
	CGlobalSynced& simulation;
	CGlobalUnsynced& client;
	IVideoCapturing& captureTiming;
	LifecycleRequests& requests;
};
struct GameDisplayContext {
	const InvocationContext& invocation;
	const GameWorldView& world;
	CGame& game;
	CGlobalUnsynced& client;
	CCamera& camera;
	CMouseHandler& mouse;
	CEventHandler& events;
	CInfoConsole& console;
	ISound& sound;
	GraphicsAccess& graphics;
	PublishedFrameLease latestSimulation;
	GameFrame& output;
};
struct GameRenderContext {
	const InvocationContext& invocation;
	const GameWorldView& world;
	const GameFrame& frame;
	CEventHandler& drawEvents;
	IVideoCapturing& capture;
	GraphicsAccess& graphics;
};
/* GameFrame is a mode-local preparation contract, not an implemented snapshot.
 * Its eventual contents must carry the source activation and required original
 * time samples. Display writes the caller-owned invocation frame; render only
 * reads it, and only for the same activation. Private jobs/helpers still require
 * adaptation; naming them in a context grants no new CGame access.
 *
 * latestSimulation is optional while serial legacy reads remain. Render's world
 * is still a live borrow and therefore cannot be submitted asynchronously. The
 * owning publication handle is the separate lifetime contract for future consumers.
 * Input/session have command/authority access; render gets neither. Reused legacy
 * callbacks may still reach globals; this contract does not claim to enforce purity.
 */
}
