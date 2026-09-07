/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <type_traits>
#include "Modes/ModeBinding.h"
#include "Modes/Game/GameMode.h"
#include "Modes/Game/Simulation/Publication/PublicationContext.h"
#include "Modes/Loading/LoadingMode.h"
#include "Modes/PreGame/PreGameMode.h"
#include "Modes/LuaMenu/LuaMenuMode.h"
#include "Modes/SelectMenu/SelectMenuMode.h"
#include "Globals/Graphics/GraphicsContext.h"

using namespace runtime;

// Each phase has its own dependency contract, not interchangeable bags of globals.
static_assert(std::is_same_v<decltype(&GameMode::Input), void (GameMode::*)(const ModeInputContext&)>);
static_assert(std::is_same_v<decltype(&GameMode::Session), void (GameMode::*)(const ModeSessionContext&)>);
static_assert(std::is_same_v<decltype(&GameMode::Display), void (GameMode::*)(const ModeDisplayContext&)>);
static_assert(std::is_same_v<decltype(&GameMode::Render), void (GameMode::*)(const ModeRenderContext&)>);
static_assert(!std::is_convertible_v<GameSessionContext, GameRenderContext>);
static_assert(!std::is_default_constructible_v<GameRenderContext>);
static_assert(!std::is_default_constructible_v<GameSessionContext>);
static_assert(!std::is_default_constructible_v<SelectMenuInputContext>);
static_assert(!std::is_default_constructible_v<LuaMenuDisplayContext>);
static_assert(!std::is_default_constructible_v<PreGameSessionContext>);

// Live world access and prepared-frame access are explicitly read-only in render.
static_assert(std::is_same_v<decltype(GameWorldView::simulation), const CGlobalSynced&>);
static_assert(std::is_same_v<decltype(GameWorldView::units), const CUnitHandler&>);
static_assert(std::is_same_v<decltype(GameDisplayContext::output), GameFrame&>);
static_assert(std::is_same_v<decltype(GameRenderContext::frame), const GameFrame&>);
static_assert(std::is_same_v<decltype(GameSessionContext::simulation), CGlobalSynced&>);
template<class T> concept HasNetwork = requires(T& context) { context.network; };
static_assert(HasNetwork<GameSessionContext>);
static_assert(!HasNetwork<GameRenderContext>);

// A published lease owns const state; a live world view carries no such promise.
static_assert(std::is_same_v<PublishedFrameLease::element_type, const PublishedSimFrame>);
static_assert(std::is_same_v<decltype(PublicationContext::output), PublishedFrameLease&>);
static_assert(std::is_same_v<decltype(PublicationContext::cachedChecksum), std::optional<std::uint32_t>>);
static_assert(std::is_same_v<decltype(LoadingSessionContext::game), const CGame&>);
static_assert(std::is_same_v<decltype(LoadingRenderContext::intro), CLuaIntro*>);
static_assert(std::is_same_v<decltype(LoadingRenderContext::origin), LoadingInvocation>);
static_assert(std::is_same_v<decltype(PresentContext::origin), PresentationOrigin>);

static_assert(std::is_same_v<decltype(ActiveModeBinding::mode), IMode*>);
static_assert(std::is_base_of_v<IMode, GameMode>);
static_assert(std::is_base_of_v<IMode, LoadingMode>);
static_assert(std::is_base_of_v<IMode, PreGameMode>);
static_assert(std::is_base_of_v<IMode, LuaMenuMode>);
static_assert(std::is_base_of_v<IMode, SelectMenuMode>);
static_assert(std::is_abstract_v<IMode>);
static_assert(std::is_constructible_v<ModeInputContext, GameInputContext>);
static_assert(std::is_constructible_v<ModeSessionContext, GameSessionContext>);
