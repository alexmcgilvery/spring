/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyLoadingServices.h"

//FIXME [LOAD-008] All statements below are retained implementation candidates,
// not executable adapters. Private owner access, helper extraction and lifetime
// contracts must be resolved before removing this guard. The narrow operations
// mirror LoadScreen.cpp; whole controller Update/Draw are never dispatched here.
#if 0
#include <algorithm>
#include <cassert>
#include <functional>
#include <mutex>
#include <string>
#include <utility>

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
#if !defined(HEADLESS) && !defined(NO_SOUND)
#include "System/Sound/OpenAL/EFX.h"
#include "System/Sound/OpenAL/EFXPresets.h"
#endif

namespace runtime {
namespace {
class LegacyLoadingProgressScope final : public LoadingProgressScope {
public:
	explicit LegacyLoadingProgressScope(spring::recursive_mutex& mutex)
		: lock(mutex)
	{}
private:
	std::lock_guard<spring::recursive_mutex> lock;
};
}

CLoadScreen& LegacyLoadingServices::Backing() const
{
	//FIXME [LOAD-009] An assertion is not a lifetime lease. Source callbacks can
	// replace owners; this private singleton read requires an explicit binding
	// contract before activation, including same-address reuse and retirement.
	assert(CLoadScreen::singleton != nullptr);
	return *CLoadScreen::singleton;
}

void LegacyLoadingServices::BeginLoading(std::string&& mapFileName, std::string&& modFileName, ILoadSaveHandler* saveFile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// [ allocation ] Keep constructor-owned strings, save pointer, thread and
	// timestamp state in CLoadScreen; the adapter owns no duplicate loading state.
	assert(CLoadScreen::singleton == nullptr);
	CLoadScreen::singleton = new CLoadScreen(std::move(mapFileName), std::move(modFileName), saveFile);

	// [ startup ] Synchronous completion happens here, before ordinary updates.
	if (InitializeLoading())
		return;

	RetireLoadingController();
	AnnounceLoadingCompletion();
}

bool LegacyLoadingServices::InitializeLoading()
{
	RECOIL_DETAILED_TRACY_ZONE;
	activeController = &Backing();
	skirmishAIHandler.LoadPreGame();
	ConfigureLoadingThread();
	StartHeartbeatAndGame();
	StartLoadingWorker();
	InitializeLoadingIntro();

	if (Backing().mtLoading)
		return true;

	LOG("[LoadScreen::%s] single-threaded", "Init");
	game->Load(Backing().mapFileName);
	return false;
}

void LegacyLoadingServices::ConfigureLoadingThread()
{
#ifdef HEADLESS
	Backing().mtLoading = false;
#else
	const int mtCfg = configHandler->GetInt("LoadingMT");
	Backing().mtLoading = (mtCfg > 0);
#endif
}

void LegacyLoadingServices::StartHeartbeatAndGame()
{
	clientNet->KeepUpdating(true);
	Backing().netHeartbeatThread = spring::thread(Threading::CreateNewThread(std::bind(&CNetProtocol::UpdateLoop, clientNet)));
	game = new CGame(Backing().mapFileName, Backing().modFileName, Backing().saveFile);
}

void LegacyLoadingServices::StartLoadingWorker()
{
	CglFont::sync.SetThreadSafety(Backing().mtLoading);
	CLoadLock::SetThreadSafety(Backing().mtLoading);
	if (Backing().mtLoading) {
		try {
			//FIXME [LOAD-003] These are the actual worker-start/fallback statements.
			// GameLoadThread's context comments are not proof of context ownership.
			// Validate actual caller context and watchdog startup/failure behavior.
			Backing().gameLoadThread = CGameLoadThread(std::bind(&CGame::Load, game, Backing().mapFileName));
			while (!Watchdog::HasThread(WDT_LOAD));
		} catch (const opengl_error& gle) {
			LOG_L(L_WARNING, "[LoadScreen::%s] offscreen GL context creation failed (error: \"%s\")", "Init", gle.what());
			Backing().mtLoading = false;
			CglFont::sync.SetThreadSafety(false);
			CLoadLock::SetThreadSafety(false);
		}
	}
}

void LegacyLoadingServices::InitializeLoadingIntro()
{
	// Lua intro creation retains the original main-thread load-lock scope.
	auto lock = CLoadLock::GetUniqueLock();
	CLuaIntro::LoadFreeHandler();
}

void LegacyLoadingServices::DeliverProgressNotifications()
{
	if (luaIntro != nullptr) {
		std::lock_guard<spring::recursive_mutex> lck(Backing().mutex);
		//FIXME [LOAD-001] Keep the exact source iteration visible. A reentrant
		// SetLoadMessage can append to this vector while a callback is running;
		// snapshotting it would change order, so neither policy is enabled yet.
		for (const auto& pair: Backing().loadMessages) {
			good_fpu_control_registers(pair.first.c_str());
			luaIntro->LoadProgress(pair.first, pair.second);
		}
		Backing().loadMessages.clear();
	}
}

bool LegacyLoadingServices::IsGameLoadingComplete() const
{
	return game->IsDoneLoading();
}

bool LegacyLoadingServices::IsMultithreadedLoading() const
{
	return Backing().mtLoading;
}

void LegacyLoadingServices::KeepWindowResponsive()
{
	// [ input ] This call pushes window events and services the watchdog even
	// though it is reached from loading display/progress, not the outer loop.
	//FIXME [LOAD-011] Moving this into ordinary input collection loses ST window
	// responsiveness while Game::Load owns the stack. Retain this exact call until
	// the loading-progress entry has an explicit input-service capability; test
	// resize, cancellation and queued input during synchronous loading.
	spring::UnfreezeSpring(WDT_LOAD);
}

std::unique_ptr<LoadingProgressScope> LegacyLoadingServices::QueueProgressNotification(std::string_view text, bool replaceLast)
{
	auto scope = std::make_unique<LegacyLoadingProgressScope>(Backing().mutex);
	const std::string ownedText(text);
	Backing().loadMessages.emplace_back(ownedText, replaceLast);
	LOG("[LoadScreen::%s] text=\"%s\"", "SetLoadMessage", ownedText.c_str());
	LOG_CLEANUP();
	good_fpu_control_registers(ownedText.c_str());
	//FIXME [LOAD-001] This scope must outlive synchronous update/render exactly
	// as SetLoadMessage's lock does, but cannot unlock a retired owner's mutex.
	return scope;
}

void LegacyLoadingServices::PaceLoadingFrame()
{
	if (Backing().mtLoading) {
		const spring_time now = spring_gettime();
		const unsigned diffTime = spring_tomsecs(now - Backing().lastDrawTime);
		constexpr unsigned wantedFPS = 50;
		constexpr unsigned minFrameTime = 1000 / wantedFPS;
		if (diffTime < minFrameTime)
			spring_sleep(spring_msecs(minFrameTime - diffTime));
		Backing().lastDrawTime = now;
	}
}

void LegacyLoadingServices::AdvanceDrawCounter()
{
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
}

void LegacyLoadingServices::MaintainLobbyConnection()
{
	if (luaMenu != nullptr)
		luaMenu->Update();
}

bool LegacyLoadingServices::HasLoadingIntro() const
{
	return luaIntro != nullptr;
}

void LegacyLoadingServices::UpdateLoadingIntro()
{
	// [ display ] Source Draw performs this inside the intro eligibility branch.
	//FIXME [LOAD-007] Extracting display ahead of pacing/lobby/eligibility or out
	// of the established loading context changes the source flow. Keep this
	// exact call immediately before DrawGenesis until its scope is validated.
	luaIntro->Update();
}

void LegacyLoadingServices::DrawLoadingGenesis()
{
	luaIntro->DrawGenesis();
}

void LegacyLoadingServices::ClearLoadingScreen()
{
	//FIXME [LOAD-010] ClearScreen is a protected CGameController operation.
	// Expose that same helper narrowly during extraction; do not duplicate its
	// viewport/clear setup here or move it ahead of DrawGenesis.
	Backing().ClearScreen();
}

void LegacyLoadingServices::DrawLoadingScreen()
{
	luaIntro->DrawLoadScreen();
}

void LegacyLoadingServices::PresentSynchronousLoadingFrame()
{
	// [ present ] The source ST draw owns this swap; MT draw does not.
	//FIXME [LOAD-006] Keep this exact internal present blocked out with the
	// adapter. Moving it to an unconditional outer presenter drops progress-only
	// frames or duplicates their swap. Trace both call routes before relocation.
	globalRendering->SwapBuffers(true, false);
}

void LegacyLoadingServices::StopLoadingResources()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (Backing().mtLoading && !Backing().gameLoadThread.joinable())
		return;
	if (luaIntro != nullptr)
		luaIntro->Shutdown();
	CLuaIntro::FreeHandler();
	Backing().gameLoadThread.join();
	CFontTexture::sync.SetThreadSafety(false);
	CLoadLock::SetThreadSafety(false);
	globalRendering->MakeCurrentContext(false);
	globalRendering->ToggleMultisampling();
}

void LegacyLoadingServices::StopHeartbeatAndActivateGame()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!Backing().gameLoadThread.joinable());
	if (clientNet != nullptr)
		clientNet->KeepUpdating(false);
	if (Backing().netHeartbeatThread.joinable())
		Backing().netHeartbeatThread.join();
	if (!gu->globalQuit) {
		activeController = game;
		if (luaMenu != nullptr)
			luaMenu->ActivateGame();
	}
	if (activeController == &Backing())
		activeController = nullptr;
}

void LegacyLoadingServices::RetireLoadingController()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (CLoadScreen::singleton == nullptr)
		return;
	StopLoadingResources();
	StopHeartbeatAndActivateGame();
	//FIXME [LOAD-002] StopHeartbeatAndActivateGame is the extracted destructor
	// body, not an additional pre-delete action. Enabling this code requires
	// removing that body from CLoadScreen's destructor or it runs twice. Preserve
	// member destruction and GameController cleanup; verify partial-init paths.
	spring::SafeDelete(CLoadScreen::singleton);
}

void LegacyLoadingServices::AnnounceLoadingCompletion()
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

void LegacyLoadingServices::ResizeLoadingIntro()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaIntro != nullptr)
		luaIntro->ViewResize();
}

int LegacyLoadingServices::PressLoadingKey(int keyCode, int scanCode, bool isRepeat)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaIntro != nullptr)
		luaIntro->KeyPress(keyCode, scanCode, isRepeat);
	return 0;
}

int LegacyLoadingServices::ReleaseLoadingKey(int keyCode, int scanCode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaIntro != nullptr)
		luaIntro->KeyRelease(keyCode, scanCode);
	return 0;
}
}
#endif
