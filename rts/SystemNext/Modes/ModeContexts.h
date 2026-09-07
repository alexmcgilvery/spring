/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
#include "SelectMenu/SelectMenuContext.h"
#include "LuaMenu/LuaMenuContext.h"
#include "PreGame/PreGameContext.h"
#include "Loading/LoadingContext.h"
#include "Game/GameContext.h"
#include <variant>
namespace runtime {
/** One concrete dependency bundle per call, not a union of all engine globals.
 * The shared interface dispatches a variant; each mode checks its own alternative.
 * Bundles copy references/metadata handles, never the referenced legacy state.
 */
using ModeInputContext = std::variant<SelectMenuInputContext, LuaMenuInputContext,
	PreGameInputContext, LoadingInputContext, GameInputContext>;
using ModeSessionContext = std::variant<PreGameSessionContext, LoadingSessionContext, GameSessionContext>;
using ModeDisplayContext = std::variant<LuaMenuDisplayContext, LoadingDisplayContext, GameDisplayContext>;
using ModeRenderContext = std::variant<SelectMenuRenderContext, LuaMenuRenderContext,
	PreGameRenderContext, LoadingRenderContext, GameRenderContext>;
}
