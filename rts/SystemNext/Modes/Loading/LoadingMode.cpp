/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LoadingMode.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

#include "Game/LoadScreen.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/MouseHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Lua/LuaIntro.h"
#include "Lua/LuaMenu.h"
#include "Map/MapInfo.h"
#include "Net/Protocol/NetProtocol.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Path/IPathManager.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/LoadLock.h"
#include "System/Log/ILog.h"
#include "System/Misc/TracyDefs.h"
#include "System/Misc/UnfreezeSpring.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Watchdog.h"
#include "System/SafeUtil.h"
#include "System/Sync/FPUCheck.h"
#include "SystemNext/Presentation/Present.h"
#if !defined(HEADLESS) && !defined(NO_SOUND)
#include "System/Sound/OpenAL/EFX.h"
#include "System/Sound/OpenAL/EFXPresets.h"
#endif

namespace runtime {
namespace {
/** Ephemeral draw state; no ownership of the controller or intro is transferred. */
struct LoadingFrameData final : ModeFrameData {
	explicit LoadingFrameData(CLoadScreen* owner)
		: controller(owner)
		, controllerId(owner->InstanceId())
#if defined(TRACY_ENABLE) && defined(RECOIL_DETAILED_TRACY_ZONING)
		, drawScope(&drawLocation, true)
#endif
	{}
	CLoadScreen* controller;
	std::uint64_t controllerId;
	CLuaIntro* intro = nullptr;
#if defined(TRACY_ENABLE) && defined(RECOIL_DETAILED_TRACY_ZONING)
	inline static constexpr tracy::SourceLocationData drawLocation = {nullptr, "CLoadScreen::Draw", __FILE__, __LINE__, 0};
	tracy::ScopedZone drawScope;
#endif
};

struct ProgressDeliveryScope {
	explicit ProgressDeliveryScope(bool& value): active(value) { active = true; }
	~ProgressDeliveryScope() { active = false; }
	bool& active;
};

struct ProgressCallbackScope {
	explicit ProgressCallbackScope(unsigned& value): depth(value) { ++depth; }
	~ProgressCallbackScope() { --depth; }
	unsigned& depth;
};
}

bool LoadingMode::HandlesSession() const
{
	return true;
}

DisplayPhase LoadingMode::GetDisplayPhase() const
{
	return DisplayPhase::WithGraphics;
}

SessionUpdate LoadingMode::UpdateSession(Session&)
{
	ZoneScoped;
	auto* owner = static_cast<CLoadScreen*>(Controller());
	if (owner == nullptr || owner != CLoadScreen::GetInstance())
		return SessionUpdate::Incomplete("LOAD-009", "Loading session has no current backing controller");
	auto& controller = *owner;
	{
		std::lock_guard<spring::recursive_mutex> lock(controller.mutex);
		if (controller.incompleteId != nullptr)
			return SessionUpdate::Incomplete(controller.incompleteId, controller.incompleteReason);
	}

	// [ progress notifications ] Keep display notifications before completion,
	// at their established session invocation point, without moving their context.
	if (luaIntro != nullptr) {
		std::lock_guard<spring::recursive_mutex> lck(controller.mutex);
		if (controller.deliveringProgress)
			return SessionUpdate::Incomplete("LOAD-001", "Recursive loading progress delivery requires a queue ordering decision");
		ProgressDeliveryScope delivery(controller.deliveringProgress);
		for (const auto& pair: controller.loadMessages) {
			good_fpu_control_registers(pair.first.c_str());
			auto* notifiedIntro = luaIntro;
			notifiedIntro->LoadProgress(pair.first, pair.second);
			if (controller.incompleteId != nullptr)
				return SessionUpdate::Incomplete(controller.incompleteId, controller.incompleteReason);
			if (notifiedIntro != luaIntro) {
				controller.incompleteId = "LOAD-007";
				controller.incompleteReason = "Loading intro changed during progress notification";
				return SessionUpdate::Incomplete(controller.incompleteId, controller.incompleteReason);
			}
		}
		controller.loadMessages.clear();
	}

	// [ session completion ] Progress owns a lock in controller storage. Stop
	// before deleting that storage; an outer iteration may not claim completion
	// of an incomplete synchronous callback.
	if (game->IsDoneLoading()) {
		if (controller.progressDepth != 0) {
			//FIXME [LOAD-005] Source deletes here even with SetLoadMessage's lock
			// held. The exact retirement operations remain below; this affected
			// path requires a deferred-retirement policy before it can run safely.
			controller.incompleteId = "LOAD-005";
			controller.incompleteReason = "Loading completed inside a progress callback holding controller storage";
			return SessionUpdate::Incomplete(controller.incompleteId, controller.incompleteReason);
		}
		RetireLoadingController();
		AnnounceLoadingCompletion();
		return SessionUpdate::FromContinuation(true);
	}

	// [ input ] Keep responsiveness while synchronous loading owns the stack.
	if (!controller.mtLoading)
		spring::UnfreezeSpring(WDT_LOAD);
	return SessionUpdate::FromContinuation(true);
}

ApplicationStatus LoadingMode::UpdateDisplay(ModeFrame& frame)
{
	auto* owner = static_cast<CLoadScreen*>(Controller());
	if (owner == nullptr || owner != CLoadScreen::GetInstance())
		return frame.Block("LOAD-009", "Loading display has no current backing controller");
	frame.data = std::make_unique<LoadingFrameData>(owner);
	auto& draw = static_cast<LoadingFrameData&>(*frame.data);
	auto& controller = *owner;
	if (!controller.mtLoading && controller.progressDepth == 0) {
		//FIXME [LOAD-006] The source ordinary ST draw had an internal swap plus
		// its caller's outer swap. Only progress execution has a proven single
		// present route. Do not silently choose a new count for an ordinary ST
		// loading frame; normal ST startup retires before reaching this path.
#if 0
		// Retained source-internal swap; ordinary execution also has an outer present.
		globalRendering->SwapBuffers(true, false);
#endif
		return frame.Block("LOAD-006", "Ordinary synchronous loading presentation requires a swap-count decision");
	}

	// [ timing ] Preserve the source pre-sleep timestamp assignment.
	if (controller.mtLoading) {
		const spring_time now = spring_gettime();
		const unsigned diffTime = spring_tomsecs(now - controller.lastDrawTime);
		constexpr unsigned wantedFPS = 50;
		constexpr unsigned minFrameTime = 1000 / wantedFPS;
		if (diffTime < minFrameTime)
			spring_sleep(spring_msecs(minFrameTime - diffTime));
		controller.lastDrawTime = now;
	}
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);

	// [ display ] Lobby maintenance precedes one intro eligibility decision.
	if (luaMenu != nullptr)
		luaMenu->Update();
	if (owner != CLoadScreen::GetInstance() || draw.controllerId != CLoadScreen::GetInstance()->InstanceId())
		return frame.Block("LOAD-007", "Loading controller changed during lobby maintenance");
	draw.intro = luaIntro;
	if (draw.intro != nullptr) {
		draw.intro->Update();
		if (owner != CLoadScreen::GetInstance() || draw.controllerId != CLoadScreen::GetInstance()->InstanceId() || draw.intro != luaIntro)
			return frame.Block("LOAD-007", "Loading intro or controller changed during display update");
	}
	return ApplicationStatus::Continue;
}

RenderResult LoadingMode::Render(ModeFrame& frame)
{
	auto* draw = dynamic_cast<LoadingFrameData*>(frame.data.get());
	if (draw == nullptr)
		return RenderResult::Blocked("LOAD-007", "Loading rendering requires prepared display state");
	if (draw->controller != CLoadScreen::GetInstance() || draw->controllerId != CLoadScreen::GetInstance()->InstanceId() || draw->intro != luaIntro)
		return RenderResult::Blocked("LOAD-007", "Loading rendering backing changed after display preparation");

	// [ rendering ] Reuse the display eligibility fact; no second intro update.
	if (draw->intro != nullptr) {
		draw->intro->DrawGenesis();
		if (draw->controller != CLoadScreen::GetInstance() || draw->controllerId != CLoadScreen::GetInstance()->InstanceId() || draw->intro != luaIntro)
			return RenderResult::Blocked("LOAD-007", "Loading backing changed during genesis callback");
		ClearScreen();
		draw->intro->DrawLoadScreen();
	}
	// Present is the caller's responsibility for ordinary and progress frames.
	return RenderResult::Ready();
}

SessionUpdate LoadingMode::ReportProgress(CLoadScreen& controller, const std::string& text, bool replaceLast, Session& session)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// [ input ] MT and ST retain window/watchdog service before locking.
	spring::UnfreezeSpring(WDT_LOAD);
	std::lock_guard<spring::recursive_mutex> lck(controller.mutex);
	if (controller.incompleteId != nullptr)
		return SessionUpdate::Incomplete(controller.incompleteId, controller.incompleteReason);
	if (controller.deliveringProgress) {
		//FIXME [LOAD-001] Appending to loadMessages from LoadProgress can
		// invalidate the active vector iteration. Keep the original append below,
		// but block this reentrant path until queue semantics are decided.
		controller.incompleteId = "LOAD-001";
		controller.incompleteReason = "A loading notification reentered active progress delivery";
		return SessionUpdate::Incomplete(controller.incompleteId, controller.incompleteReason);
	}

	// [ progress ] Queue, log/cleanup and FPU order remain unchanged.
	controller.loadMessages.emplace_back(text, replaceLast);
	LOG("[LoadScreen::%s] text=\"%s\"", "SetLoadMessage", text.c_str());
	LOG_CLEANUP();
	good_fpu_control_registers(text.c_str());
	if (controller.mtLoading)
		return SessionUpdate::FromContinuation(true);

	// [ synchronous frame ] ST callbacks execute on the established caller
	// context. MT never touches the shared mode binding from its producer thread.
	ProgressCallbackScope callback(controller.progressDepth);
	BindController(&controller);
	auto update = UpdateSession(session);
	if (update.applicationStatus == ApplicationStatus::Blocked)
		return update;
	ModeFrame frame;
	if (UpdateDisplay(frame) == ApplicationStatus::Blocked) {
		controller.incompleteId = "LOAD-007";
		controller.incompleteReason = "Synchronous loading display did not complete";
		return SessionUpdate::Incomplete(frame.blocked.id, frame.blocked.reason);
	}
	const auto rendered = Render(frame);
	if (rendered.state == RenderState::Blocked) {
		controller.incompleteId = "LOAD-007";
		controller.incompleteReason = "Synchronous loading rendering did not complete";
		return SessionUpdate::Incomplete(rendered.blocked.id, rendered.blocked.reason);
	}

	// [ present ] Exactly one progress present, through the shared presenter.
	// The frame's draw scope remains alive through this source-internal present.
	PresentWindow(rendered.AllowPresent());
	return SessionUpdate::FromContinuation(true);
}

void LoadingMode::BeginLoading(std::string&& mapFileName, std::string&& modFileName, ILoadSaveHandler* saveFile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Startup is explicit: ST loading can finish before the first ordinary update.
	assert(CLoadScreen::singleton == nullptr);
	CLoadScreen::singleton = new CLoadScreen(std::move(mapFileName), std::move(modFileName), saveFile);
	BindController(CLoadScreen::singleton);
	if (InitializeLoading())
		return;
	RetireLoadingController();
	AnnounceLoadingCompletion();
}

bool LoadingMode::InitializeLoading()
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& controller = *static_cast<CLoadScreen*>(Controller());
	SetActiveController(&controller);
	skirmishAIHandler.LoadPreGame();
#ifdef HEADLESS
	controller.mtLoading = false;
#else
	const int mtCfg = configHandler->GetInt("LoadingMT");
	controller.mtLoading = (mtCfg > 0);
#endif

	// [ session loading ] Keep transport alive while constructing/loading game.
	clientNet->KeepUpdating(true);
	controller.netHeartbeatThread = spring::thread(Threading::CreateNewThread(std::bind(&CNetProtocol::UpdateLoop, clientNet)));
	game = new CGame(controller.mapFileName, controller.modFileName, controller.saveFile);
	CglFont::sync.SetThreadSafety(controller.mtLoading);
	CLoadLock::SetThreadSafety(controller.mtLoading);
	if (controller.mtLoading) {
		try {
			//FIXME [LOAD-003] Comments describe context creation that WrapFunc does
			// not perform. Validate actual context and startup failure behavior;
			// preserve these exact start/wait/fallback statements in the meantime.
			controller.gameLoadThread = CGameLoadThread(std::bind(&CGame::Load, game, controller.mapFileName));
			while (!Watchdog::HasThread(WDT_LOAD));
		} catch (const opengl_error& gle) {
			LOG_L(L_WARNING, "[LoadScreen::%s] offscreen GL context creation failed (error: \"%s\")", "Init", gle.what());
			controller.mtLoading = false;
			CglFont::sync.SetThreadSafety(false);
			CLoadLock::SetThreadSafety(false);
		}
	}

	// [ display initialization ] Intro uses its established main-thread context.
	{
		auto lock = CLoadLock::GetUniqueLock();
		CLuaIntro::LoadFreeHandler();
	}
	if (controller.mtLoading)
		return true;
	LOG("[LoadScreen::%s] single-threaded", "Init");
	game->Load(controller.mapFileName);
	if (controller.incompleteId != nullptr)
		throw IncompleteFlow(controller.incompleteId, controller.incompleteReason);
	return false;
}

void LoadingMode::StopLoadingResources()
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& controller = *static_cast<CLoadScreen*>(Controller());
	if (controller.mtLoading && !controller.gameLoadThread.joinable())
		return;
	if (luaIntro != nullptr)
		luaIntro->Shutdown();
	CLuaIntro::FreeHandler();
	controller.gameLoadThread.join();
	CFontTexture::sync.SetThreadSafety(false);
	CLoadLock::SetThreadSafety(false);
	globalRendering->MakeCurrentContext(false);
	globalRendering->ToggleMultisampling();
}

void LoadingMode::FinishControllerDestruction()
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& controller = *static_cast<CLoadScreen*>(Controller());
	assert(!controller.gameLoadThread.joinable());
	if (clientNet != nullptr)
		clientNet->KeepUpdating(false);
	if (controller.netHeartbeatThread.joinable())
		controller.netHeartbeatThread.join();
	if (!gu->globalQuit) {
		SetActiveController(game);
		if (luaMenu != nullptr)
			luaMenu->ActivateGame();
	}
	if (activeController == &controller)
		SetActiveController(nullptr);
}

void LoadingMode::RetireLoadingController()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (CLoadScreen::singleton == nullptr)
		return;
	StopLoadingResources();
	// The destructor reverse-forwards its body to FinishControllerDestruction;
	// member/base destruction therefore occurs exactly once in its original order.
	spring::SafeDelete(CLoadScreen::singleton);
	BindController(nullptr);
}

void LoadingMode::AnnounceLoadingCompletion()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (gu->globalQuit)
		return;
	const CPlayer* p = playerHandler.Player(gu->myPlayerNum);
	clientNet->Send(CBaseNetProtocol::Get().SendPlayerName(gu->myPlayerNum, p->name));
#ifdef SYNCCHECK
	clientNet->Send(CBaseNetProtocol::Get().SendPathCheckSum(gu->myPlayerNum, pathManager->GetPathCheckSum()));
#endif
	mouse->ShowMouse();
#if !defined(HEADLESS) && !defined(NO_SOUND)
	efx.CommitEffects(mapInfo->efxprops);
#endif
}

void LoadingMode::ResizeEvent()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaIntro != nullptr)
		luaIntro->ViewResize();
}

int LoadingMode::KeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaIntro != nullptr)
		luaIntro->KeyPress(keyCode, scanCode, isRepeat);
	return 0;
}

int LoadingMode::KeyReleased(int keyCode, int scanCode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaIntro != nullptr)
		luaIntro->KeyRelease(keyCode, scanCode);
	return 0;
}
}
