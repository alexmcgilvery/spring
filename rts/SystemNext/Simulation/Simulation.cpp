/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SystemNext/Diagnostics/LegacyRuntimeDiagnostics.h"
#include "Rendering/GL/myGL.h"

#include <Rml/Backends/RmlUi_Backend.h>
#include <RmlUi/Core.h>
#include "Simulation.h"
#include "Game/Game.h"
#include "SystemNext/Modes/Game/GameMode.h"
#include "SystemNext/Modes/LegacyModeBinding.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/ChatMessage.h"
#include "Game/CommandMessage.h"
#include "Game/ConsoleHistory.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/WaitCommandsAI.h"
#include "Game/WordCompletion.h"
#include "Game/IVideoCapturing.h"
#include "Game/InMapDraw.h"
#include "Game/InMapDrawModel.h"
#include "Game/SyncedActionExecutor.h"
#include "Game/SyncedGameCommands.h"
#include "Game/UnsyncedActionExecutor.h"
#include "Game/UnsyncedGameCommands.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/PlayerRoster.h"
#include "Game/UI/PlayerRosterDrawer.h"
#include "Game/UI/UnitTracker.h"
#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/WorldDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/Fonts/CFontTexture.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ModelsDataUploader.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/TeamHighlight.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/UniformConstants.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaHandle.h"
#include "Lua/LuaDebugExtra.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaMenu.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaOpenGL.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "Lua/LuaUI.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Net/GameServer.h"
#include "Net/Protocol/NetProtocol.h"
#include "Sim/Ecs/Registry.h"
#include "Sim/Ecs/Helper.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/YardmapStatusEffectsMap.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/BuildingMaskMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ResourceHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveTypeFactory.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Scripts/UnitScriptFactory.h"
#include "Sim/Units/Scripts/UnitScriptEngine.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponLoader.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/EndGameBox.h"
#include "Game/UI/GameSetupDrawer.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Game/UI/ResourceBar.h"
#include "Game/UI/SelectionKeyHandler.h"
#include "Game/UI/TooltipConsole.h"
#include "Game/UI/ProfileDrawer.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/creg/SerializeLuaState.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Sync/FPUCheck.h"
#include "System/SafeUtil.h"
#include "System/SpringExitCode.h"
#include "System/SpringMath.h"
#include "System/FileSystem/FileSystem.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Log/ILog.h"
#include "System/Platform/Misc.h"
#include "System/Platform/Watchdog.h"
#include "System/Platform/errorhandler.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/DumpState.h"
#include "System/TimeProfiler.h"
#include "System/LoadLock.h"

#include "System/Misc/TracyDefs.h"

#include "fmt/ranges.h"


#undef CreateDirectory


namespace runtime {
static const char* const tracingSimFrameName = "SimFrame";
//FIXME ADAPTER-SIMULATION: This copied flow requires private CGame timing and
// skip state. Provide adapter-owned backing access before activation. The legacy
// SimFrame body and its authoritative callers remain intact and executable.
void Simulation::StepFrame(CGame& backing)
{
	ENTER_SYNCED_CODE();
	ASSERT_SYNCED(gsRNG.GetGenState());

	DumpRNG(-1, -1);

	good_fpu_control_registers("CGame::SimFrame");

	FrameMarkStart(tracingSimFrameName);

	// note: starts at -1, first actual frame is 0
	gs->frameNum += 1;
#ifdef SYNC_HISTORY
	CSyncChecker::NewGameFrame();
#endif
	backing.lastFrameTime = spring_gettime();
	// This is not very ideal, as the timeoffset of each new draw frame is also calculated from this
	// with a strange side effect: if the timeOffset was a high number, like 0.9, then this will force the next draw frame to have an offset of 0.0x
	// What this means, is that in the case where we have frames to spare, and and over rendering, then the following can happen at 60hz:
	// simframe
	// drawframe timeOffset ~ 0.0
	// drawframe timeoffset ~ 0.5
	// drawframe timeoffset ~ 1.0 (1 extra draw!)
	// simframe
	// drawframe timeoffset ~ 0.0 // THIS is the problematic case, as visually, this frame is 'near identical' to the previously drawn one!
	// simframe
	// drawframe timeoffset ~ 0.0
	// drawframe timeoffset ~ 0.5
	// simframe
	// etc...
	// See SmoothTimeOffset for a fix to this


//FIXME SIM-INTERPOLATION-EXPERIMENT: These statements were already disabled
	// in the original tick. They adjust presentation smoothing from simulation.
	// Retain the experiment disabled; it is not required or reported as completed
	// work. Resolve sim/display timing ownership before independently scheduling.
#if 0
	if (globalRendering->timeOffset > 1.0)
		backing.lastFrameTime += spring_time::fromNanoSecs(static_cast<int64_t>((globalRendering->timeOffset - 1.0f) / globalRendering->weightedSpeedFactor * std::int64_t(1e6)));

	if (globalRendering->timeOffset < 0.0)
		backing.lastFrameTime += spring_time::fromNanoSecs(static_cast<int64_t>((globalRendering->timeOffset       ) / globalRendering->weightedSpeedFactor * std::int64_t(1e6)));
#endif

	// clear allocator statistics periodically
	// note: allocator itself should do this (so that
	// stats are reliable when paused) but see LuaUser
	spring_lua_alloc_update_stats((gs->frameNum % GAME_SPEED) == 0);

	//FIXME SIM-CLIENT-EFFECTS: These client/audio/input effects are still tick-bound.
	// Their actual statements remain synchronous here; moving them to display/input
	// changes cadence and command order and is future compatibility work.
	if (!backing.skipping) {
		// everything here is unsynced and should ideally moved to Game::Update()
		waitCommandsAI.Update();
		geometricObjects->Update();
		sound->NewFrame();
		eoh->Update();

		for (auto& grouphandler: uiGroupHandlers)
			grouphandler.Update();

		CPlayer* p = playerHandler.Player(gu->myPlayerNum);
		FPSUnitController& c = p->fpsController;

		c.SendStateUpdate(/*camera->GetMovState(), mouse->buttons*/);

		CTeamHighlight::Update(gs->frameNum);
	}

	// everything from here is simulation
	{
		SCOPED_SPECIAL_TIMER("Sim");

		// Lua unit scripts change piece positions and orientations in eventHandler.GameFrame(gs->frameNum);
		// so we need to save the previous unit state before it happened
		unitHandler.UpdatePreFrame();
		featureHandler.UpdatePreFrame();

		{
			SCOPED_TIMER("Sim::GameFrame");

			// keep garbage-collection rate tied to sim-speed
			// (fixed 30Hz gc is not enough while catching up)
			if (backing.luaGCControl == 0)
				eventHandler.CollectGarbage(false);

			eventHandler.GameFrame(gs->frameNum);
		}

		helper->Update();
		readMap->Update();
		smoothGround.UpdateSmoothMesh();
		mapDamage->Update();
		unitHandler.Update();
		pathManager->Update();
		projectileHandler.Update();
		featureHandler.Update();
		{
			/* The default GAME_SPEED is 30, which doesn't divide 1000 well,
			 * so scripts will perceive 990ms per second. But this is fine,
			 * since doing "29th February" style of extra counting would be
			 * disruptive to sleeps that assume a constant tick length while
			 * not being otherwise perceptible since most animations don't
			 * run that long. */
			static constexpr int tickMs = 1000 / GAME_SPEED;

			SCOPED_TIMER("Sim::Script");
			unitScriptEngine->Tick(tickMs);

			unitHandler.UpdatePostAnimation();
		}
		envResHandler.Update();
		losHandler->Update();
		// dead ghosts have to be updated in sim, after los,
		// to make sure they represent the current knowledge correctly.
		// should probably be split from drawer
		CUnitDrawer::UpdateGhostedBuildings();
		interceptHandler.Update(false);

		teamHandler.GameFrame(gs->frameNum);
		playerHandler.GameFrame(gs->frameNum);
		eventHandler.GameFramePost(gs->frameNum);
	}

	backing.lastSimFrameTime = spring_gettime();
	gu->avgSimFrameTime = mix(gu->avgSimFrameTime, (backing.lastSimFrameTime - backing.lastFrameTime).toMilliSecsf(), 0.05f);
	gu->avgSimFrameTime = std::max(gu->avgSimFrameTime, 0.01f);

	eventHandler.DbgTimingInfo(TIMING_SIM, backing.lastFrameTime, backing.lastSimFrameTime);

	FrameMarkEnd(tracingSimFrameName);

	#ifdef HEADLESS
	{
		const float msecMaxSimFrameTime = 1000.0f / (GAME_SPEED * gs->wantedSpeedFactor);
		const float msecDifSimFrameTime = (backing.lastSimFrameTime - backing.lastFrameTime).toMilliSecsf();
		// multiply by 0.5 to give unsynced code some execution time (50% of our sleep-budget)
		const float msecSleepTime = (msecMaxSimFrameTime - msecDifSimFrameTime) * 0.5f;

		if (msecSleepTime > 0.0f) {
			spring_sleep(spring_msecs(msecSleepTime));
		}
	}
	#endif

	// useful for desync-debugging (enter instead of -1 start & end frame of the range you want to debug)
	DumpState(-1, -1, 1, std::nullopt);

	ASSERT_SYNCED(gsRNG.GetGenState());
	LEAVE_SYNCED_CODE();
}
}
