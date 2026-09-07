/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Globals/InvocationContext.h"
#include "../../Globals/Graphics/GraphicsContext.h"
#include <span>
union SDL_Event;
class CLoadScreen;
class CGame;
class CLuaIntro;
class CLuaMenu;
namespace runtime {
struct LifecycleRequests;
enum class LoadingInvocation { OrdinaryIteration, ProgressCallback };
struct LoadingInputContext {
	const InvocationContext& invocation;
	std::span<const SDL_Event> events;
	CLoadScreen& loading;
	LifecycleRequests& requests;
};
struct LoadingSessionContext {
	const InvocationContext& invocation;
	CLoadScreen& loading;
	const CGame& game;
	LoadingInvocation origin;
	LifecycleRequests& requests;
};
struct LoadingDisplayContext {
	const InvocationContext& invocation;
	CLoadScreen& loading;
	GraphicsAccess& graphics;
	LoadingInvocation origin;
	CLuaIntro* intro;
	CLuaMenu* menu;
};
struct LoadingRenderContext {
	const InvocationContext& invocation;
	GraphicsAccess& graphics;
	LoadingInvocation origin;
	CLuaIntro* intro;
	CLuaMenu* menu;
};
/* Intro/menu pointers explicitly allow absent handlers and never own them.
 * The invocation origin preserves the ordinary/progress distinction without
 * deciding reentrancy or swap policy. Rendering gets no loading worker/queue.
 * Session observes game completion; it cannot mutate game state through game. */
}
