/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameMode.h"

#include "SystemNext/Diagnostics/LegacyRuntimeDiagnostics.h"
#include "Rendering/GL/myGL.h"
#include <Rml/Backends/RmlUi_Backend.h>
#include <RmlUi/Core.h>
#include "Game/Game.h"
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

namespace runtime {
namespace {
/** Keep samples and draw-phase lifetime with one guarded display/render invocation. */
struct GameFrameData final : ModeFrameData {

	GameFrameData(): drawPhase(Phase::Draw), currentTimePreUpdate(spring_gettime()) {}
	legacy::PhaseScope drawPhase;
	const spring_time currentTimePreUpdate;
	spring_time currentTimePreDraw;
};
}

bool GameMode::HandlesSession() const
{
	return true;
}

DisplayPhase GameMode::GetDisplayPhase() const
{
	return DisplayPhase::WithGraphics;
}

/**
 * Preserve jobs/transport before the accepted stream, including capture pacing
 * and the explicit synced region. Existing callbacks and failures stay inline.
 */
SessionUpdate GameMode::UpdateSession(Session&)
{
	auto& backing = *static_cast<CGame*>(Controller());
	RECOIL_DETAILED_TRACY_ZONE;
	good_fpu_control_registers("CGame::Update");

	runtime::legacy::BeginSession();

	backing.jobDispatcher.Update();
	clientNet->Update();

	// When video recording do step by step simulation, so each simframe gets a corresponding videoframe
	// FIXME: SERVER ALREADY DOES THIS BY ITSELF
	if (backing.playing && gameServer != nullptr && videoCapturing->AllowRecord())
		gameServer->CreateNewFrame(false, true);

	ENTER_SYNCED_CODE();
	backing.SendClientProcUsage();
	backing.ClientReadNet(); // issues new SimFrame()s

	if (!backing.gameOver) {
		if (clientNet->NeedsReconnect())
			clientNet->AttemptReconnect(SpringVersion::GetSync(), Platform::GetPlatformStr());

		if (clientNet->CheckTimeout(0, gs->PreSimFrame()))
			backing.GameEnd({}, true);
	}

	LEAVE_SYNCED_CODE();

	{
		SLuaAllocError error = {};

		if (spring_lua_alloc_get_error(&error)) {
			// convert the "abc\ndef\n..." buffer into 0-terminated "abc", "def", ... chunks
			for (char *ptr = &error.msgBuf[0], *tmp = nullptr; (tmp = strstr(ptr, "\n")) != nullptr; ptr = tmp + 1) {
				*tmp = 0;

				LOG_L(L_FATAL, "%s", error.msgBuf);
				CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_FATAL, error.msgBuf);

				// force a restart if synced Lua died, simply reloading might not work
				gu->globalQuit = gu->globalQuit || (strstr(error.msgBuf, "[OOM] synced=1") != nullptr);
			}
		}
	}

	return SessionUpdate::FromContinuation(true);
}

/**
 * Maintain the live client view under graphics ownership. Skipping may draw its
 * own screen while declining ordinary render; its early return bypasses input
 * submission and all subsequent maintenance exactly as the original path did.
 */
ApplicationStatus GameMode::UpdateDisplay(ModeFrame& frame)
{
	auto& backing = *static_cast<CGame*>(Controller());
	auto invocation = std::make_unique<GameFrameData>();
	const spring_time currentTime = invocation->currentTimePreUpdate;
	frame.data = std::move(invocation);
	{

		runtime::legacy::PhaseScope presentationPhase(runtime::Phase::Presentation);
		SCOPED_TIMER("Update");

		// timings and frame interpolation
		const spring_time deltaDrawFrameTime = currentTime - globalRendering->lastFrameStart;

		const float modGameDeltaTimeSecs = mix(deltaDrawFrameTime.toMilliSecsf() * 0.001f, 0.01f, backing.skipping);
		const float unsyncedUpdateDeltaTime = (currentTime - backing.lastUnsyncedUpdateTime).toSecsf();

		{
			// update game timings
			globalRendering->lastFrameStart = currentTime;
			globalRendering->lastFrameTime = deltaDrawFrameTime.toMilliSecsf();

			gu->avgFrameTime = mix(gu->avgFrameTime, deltaDrawFrameTime.toMilliSecsf(), 0.05f);
			gu->gameTime += modGameDeltaTimeSecs;
			gu->modGameTime += (modGameDeltaTimeSecs * gs->speedFactor * (1 - gs->paused));

			backing.totalGameTime += (modGameDeltaTimeSecs * (backing.playing && !backing.gameOver));
			backing.updateDeltaSeconds = modGameDeltaTimeSecs;
		}

		{
			// update sim-FPS counter once per second
			static int lsf = gs->frameNum;
			static spring_time lsft = currentTime;

			// toSecsf throws away too much precision
			const float diffMilliSecs = (currentTime - lsft).toMilliSecsf();

			if (diffMilliSecs >= 1000.0f) {
				gu->simFPS = (gs->frameNum - lsf) / (diffMilliSecs * 0.001f);
				lsft = currentTime;
				lsf = gs->frameNum;
			}
		}

		if (backing.skipping) {
			frame.allowRender = false;
			// when fast-forwarding, maintain a draw-rate of 2Hz
			if (spring_tomsecs(currentTime - backing.skipLastDrawTime) < 500.0f)
				return ApplicationStatus::Continue;

			backing.skipLastDrawTime = currentTime;

			backing.DrawSkip();
			return ApplicationStatus::Continue;
		}

		const bool newSimFrame = (backing.lastSimFrame != gs->frameNum);
		backing.numDrawFrames++;
		globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
		globalRendering->lastFrameStart = currentTime;
		// Update the interpolation coefficient (globalRendering->timeOffset)
		if (!gs->paused && !backing.IsSimLagging() && !gs->PreSimFrame() && !videoCapturing->AllowRecord()) {
			globalRendering->weightedSpeedFactor = 0.001f * gu->simFPS;
			globalRendering->lastTimeOffset = globalRendering->timeOffset;
			globalRendering->timeOffset = (currentTime - backing.lastFrameTime).toMilliSecsf() * globalRendering->weightedSpeedFactor;

			int SmoothTimeOffset = configHandler->GetInt("SmoothTimeOffset");
			float strictness = 0.9f; // This defines how strict we are going to be when trying to keep frame timings
			if (SmoothTimeOffset > 0) {
				strictness = 1.0f - (SmoothTimeOffset) * 0.025f;
			}

			// The main issue that SmoothTimeOffset tries to fix:
			// Is that lastFrameTime is reset when a sim frame is issued.
			// This makes the calculation of the timeOffset of the next draw frame after simframe incorrect (too small),
			// if the previous draw frame had a large timeOffset

			float drawsimratio = gu->simFPS * gu->avgFrameTime * 0.001f; // This should be like 0.5 for 60hz draw 30hz sim
			float LTO = globalRendering->lastTimeOffset;
			float CTO = globalRendering->timeOffset;

			// This mode forces a strict time step of 0.5 simframes per draw frames. Only useful for testing @ 60hz
			if (SmoothTimeOffset == -1) {
				if (newSimFrame) {
					if (LTO > (1.0f - drawsimratio * strictness))
						globalRendering->timeOffset = drawsimratio;
					else
						globalRendering->timeOffset = 0.0f;
				} else {
					if (LTO > drawsimratio * strictness)
						globalRendering->timeOffset = std::fmin(LTO + drawsimratio * strictness, 1.0f);
					else
						globalRendering->timeOffset = std::fmin(drawsimratio * strictness, 1.0f);
				}
			}

			// This mode tries to correct for the wrongly calculated timeOffset adaptively,
			// while trying to maintain a smooth interpolation rate
			// As frame rates dip below 45fps, this method is only marginally better than old method
			// But that is heavily dependent on whether the load is sim or draw based.
			// TODO: the camera smoothing still seems to take sim load into account heavily. So large sim loads jitter the camera quite a bit when moving
			if (SmoothTimeOffset > 0){

				// if we have a new sim frame, then check when the time and CTO of the previous draw frame was.
				drawsimratio = std::fmin(drawsimratio, 1.0);  // Clamp it otherwise we will accumulate delay when < 30 FPS
				float oldCTO = globalRendering->timeOffset;
				float newCTO = globalRendering->timeOffset;

				if (newSimFrame) {
					// newsimframe is a special case, as our new time offset is kind of wrong.
					// What we want to know is when the last draw happened, and at what offset.
					// There are two special cases here, if the last draw happened "on time", then we want to 'pull in' CTO to 0,
					// irrespective of the time spent in sim.
					// If the last draw frame didnt happen on time, and had a large CTO, then we need to 'carry over' some time offset

					if ((LTO + drawsimratio - 1.0 > (CTO)* strictness)) {
						newCTO = std::fmin((LTO + drawsimratio - 1.0f) * strictness, 1.3f);
						//LOG_L(L_DEBUG, "UpdateUnsynced newframe skipping, last = %.3f, currtimeoffset = %.3f, averageoffset = %.3f, now cheating it to %.3f", globalRendering->lastTimeOffset, globalRendering->timeOffset, drawsimratio, newCTO);
						globalRendering->timeOffset = newCTO;
					}
				}
				else {
					// On draw frames that dont have a preceding sim frame, we want to 'smooth' the CTO out a bit.
					// Otherwise, the sim frame is also calculated into the offset, making things jittery
					if ((CTO - LTO < (drawsimratio) * strictness)) {
						newCTO = std::fmin(LTO + drawsimratio * strictness, 1.3f);
						//LOG_L(L_DEBUG, "UpdateUnsynced Too short draw offset, last = %.3f, currtimeoffset = %.3f, averageoffset = %.3f, now cheating it to %.3f", globalRendering->lastTimeOffset, globalRendering->timeOffset, drawsimratio, newCTO);
						globalRendering->timeOffset = newCTO;
					}

				}
				//LOG_L(L_DEBUG, "oldCTO = %.3f newCTO = %.3f, drawsimratio = %.3f,  newframe = %d", oldCTO, newCTO, drawsimratio, newSimFrame);
			}



		} else {
			globalRendering->timeOffset = videoCapturing->GetTimeOffset();

			backing.lastSimFrameTime = currentTime;
			backing.lastFrameTime = currentTime;
		}

		if ((currentTime - backing.frameStartTime).toMilliSecsf() >= 1000.0f) {
			globalRendering->FPS = (backing.numDrawFrames * 1000.0f) / std::max(0.01f, (currentTime - backing.frameStartTime).toMilliSecsf());

			// update draw-FPS counter once every second
			backing.frameStartTime = currentTime;
			backing.numDrawFrames = 0;

		}

		const bool forceUpdate = (unsyncedUpdateDeltaTime >= INV_GAME_SPEED);

		backing.lastSimFrame = gs->frameNum;

		// set camera
		camHandler->UpdateController(playerHandler.Player(gu->myPlayerNum), gu->fpsMode);

		lineDrawer.UpdateLineStipple();

		icon::iconHandler.Update();
		CNamedTextures::Update();

		// always update InfoTexture and SoundListener at <= 30Hz (even when paused)
		if (newSimFrame || forceUpdate) {
			backing.lastUnsyncedUpdateTime = currentTime;

			// TODO: should be moved to WorldDrawer::Update
			infoTextureHandler->Update();
			// TODO call only when camera changed
			sound->UpdateListener(camera->GetPos(), camera->GetDir(), camera->GetUp());
		}
		backing.SetDrawMode(CGame::gameNormalDraw); //TODO move to ::Draw()?

		if (luaUI != nullptr) {
			luaUI->CheckStack();
			luaUI->CheckAction();
		}
		if (luaGaia != nullptr)
			luaGaia->CheckStack();
		if (luaRules != nullptr)
			luaRules->CheckStack();

		// Input submission retains its original late position: moving it before the
		// authoritative pump would change accepted-input timing and skip behavior.
		SubmitPendingInput();

		infoConsole->PushNewLinesToEventHandler();
		infoConsole->Update();

		//infoConsole->Update() can in theory cause the need to update fonts, so update here
		CFontTexture::Update();

		mouse->Update();
		mouse->UpdateCursors();
		guihandler->Update();
		commandDrawer->Update();

		{
			SCOPED_TIMER("Update::EventHandler");
			eventHandler.Update();
		}

		if (unitTracker.Enabled())
			unitTracker.SetCam();

		camera->Update();
		shadowHandler.Update();
		{
			backing.worldDrawer.Update(newSimFrame);
			transformsUploader.Update();
			modelUniformsUploader.Update();
		}

		mouse->UpdateCursorCameraDir(); // make sure mouse->dir is in sync with camera

		//Update per-drawFrame UBO
		UniformConstants::GetInstance().Update();

		eventHandler.DbgTimingInfo(TIMING_UNSYNCED, currentTime, spring_now());
	} // presentation timer ends before document-interface update
	RmlGui::Update();
	static_cast<GameFrameData&>(*frame.data).currentTimePreDraw = spring_gettime();
	return ApplicationStatus::Continue;
}

/**
 * Render from the same invocation samples used by display. Genesis precedes
 * inactive-window eligibility; completion averages include capture but exclude
 * ordinary window present. The frame owns the surrounding diagnostic scope.
 */
RenderResult GameMode::Render(ModeFrame& frame)
{
	if (!frame.allowRender)
		return RenderResult::Skipped();
	auto& backing = *static_cast<CGame*>(Controller());
	const auto& invocation = static_cast<const GameFrameData&>(*frame.data);
	const auto currentTimePreUpdate = invocation.currentTimePreUpdate;
	const auto currentTimePreDraw = invocation.currentTimePreDraw;
	SCOPED_SPECIAL_TIMER("Draw");
	SCOPED_GL_DEBUGGROUP("Draw");
	globalRendering->SetGLTimeStamp(CGlobalRendering::FRAME_REF_TIME_QUERY_IDX);

	backing.SetDrawMode(CGame::gameNormalDraw);

	// Bind per-drawFrame UBO
	UniformConstants::GetInstance().Bind();

	{
		SCOPED_TIMER("Draw::DrawGenesis");
		eventHandler.DrawGenesis();
	}

	if (!globalRendering->active) {
		spring_sleep(spring_msecs(10));

		// return early if and only if less than 30K milliseconds have passed since last draw-frame
		// so we force render two frames per minute when minimized to clear batches and free memory
		// don't need to mess with globalRendering->active since only mouse-input code depends on it
		if ((currentTimePreDraw - backing.lastDrawFrameTime).toSecsi() < 30)
			return RenderResult::Skipped();
	}

	if (globalRendering->drawDebug) {
		const float deltaFrameTime = (currentTimePreUpdate - backing.lastSimFrameTime).toMilliSecsf();
		const float deltaNetPacketProcTime  = (currentTimePreUpdate - backing.lastNetPacketProcessTime ).toMilliSecsf();
		const float deltaReceivedPacketTime = (currentTimePreUpdate - backing.lastReceivedNetPacketTime).toMilliSecsf();
		const float deltaSimFramePacketTime = (currentTimePreUpdate - backing.lastSimFrameNetPacketTime).toMilliSecsf();

		const float currTimeOffset = globalRendering->timeOffset;
		static float lastTimeOffset = globalRendering->timeOffset;
		static int lastGameFrame = gs->frameNum;

		static const char* minFmtStr = "assert(CTO >= 0.0f) failed (SF=%u : DF=%u : CTO=%f : WSF=%f : DT=%fms : DLNPPT=%fms | DLRPT=%fms | DSFPT=%fms : NP=%u)";
		static const char* maxFmtStr = "assert(CTO <= 1.3f) failed (SF=%u : DF=%u : CTO=%f : WSF=%f : DT=%fms : DLNPPT=%fms | DLRPT=%fms | DSFPT=%fms : NP=%u)";

		// CTO = MILLISECSF(CT - LSFT) * WSF = MILLISECSF(CT - LSFT) * (SFPS * 0.001)
		// AT 30Hz LHS (MILLISECSF(CT - LSFT)) SHOULD BE ~33ms, RHS SHOULD BE ~0.03
		assert(currTimeOffset >= 0.0f);

		if (currTimeOffset < 0.0f) LOG_L(L_DEBUG, minFmtStr, gs->frameNum, globalRendering->drawFrame, currTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime, deltaNetPacketProcTime, deltaReceivedPacketTime, deltaSimFramePacketTime, clientNet->GetNumWaitingServerPackets());
		if (currTimeOffset > 1.3f) LOG_L(L_DEBUG, maxFmtStr, gs->frameNum, globalRendering->drawFrame, currTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime, deltaNetPacketProcTime, deltaReceivedPacketTime, deltaSimFramePacketTime, clientNet->GetNumWaitingServerPackets());

		// test for monotonicity, normally should only fail
		// when SimFrame() advances time or if simframe rate
		// changes
		if (lastGameFrame == gs->frameNum && currTimeOffset < lastTimeOffset)
			LOG_L(L_DEBUG, "assert(CTO >= LTO) failed (SF=%u : DF=%u : CTO=%f : LTO=%f : WSF=%f : DT=%fms)", gs->frameNum, globalRendering->drawFrame, currTimeOffset, lastTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime);

		lastTimeOffset = currTimeOffset;
		lastGameFrame = gs->frameNum;
	}

	//FIXME move both to UpdateUnsynced?
	CTeamHighlight::Enable(spring_tomsecs(currentTimePreDraw));
	RenderWorld();
	RenderInterface();

	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();

	if (videoCapturing->AllowRecord()) {
		videoCapturing->SetLastFrameTime(globalRendering->lastFrameTime = 1000.0f / GAME_SPEED);
		// does nothing unless StartCapturing has also been called via /createvideo (Windows-only)
		videoCapturing->RenderFrame();
	}

	backing.SetDrawMode(CGame::gameNotDrawing);
	CTeamHighlight::Disable();

	const spring_time currentTimePostDraw = spring_gettime();
	const spring_time currentFrameDrawTime = currentTimePostDraw - currentTimePreDraw;
	gu->avgDrawFrameTime = mix(gu->avgDrawFrameTime, currentFrameDrawTime.toMilliSecsf(), 0.05f);

	eventHandler.DbgTimingInfo(TIMING_VIDEO, currentTimePreDraw, currentTimePostDraw);
	globalRendering->SetGLTimeStamp(CGlobalRendering::FRAME_END_TIME_QUERY_IDX);

	backing.lastDrawFrameTime = currentTimePostDraw;

	return RenderResult::Ready();
}

/** Minimap coverage never suppresses world rendering or environment preparation. */
void GameMode::RenderWorld()
{
	auto& backing = *static_cast<CGame*>(Controller());
	minimap->Update();

	// note: neither this call nor DrawWorld can be made conditional on minimap->GetMaximized()
	// minimap never covers entire screen when maximized unless map aspect-ratio matches screen
	// (unlikely);
	backing.worldDrawer.GenerateIBLTextures();

	// restore back to the default FBO / Viewport
	if (FBO::IsSupported())
		FBO::Unbind();
	camera->LoadViewport();

	backing.worldDrawer.Draw();
	backing.worldDrawer.ResetMVPMatrices();
}

/** Preserve overlay ordering and its nested timer/graphics debug scope. */
void GameMode::RenderInterface()
{
	auto& backing = *static_cast<CGame*>(Controller());
	SCOPED_TIMER("Draw::Screen");
	SCOPED_GL_DEBUGGROUP("Draw::Screen");
	if (CUnitDrawer::UseScreenIcons())
		unitDrawer->DrawUnitIconsScreen();

	eventHandler.DrawScreenEffects();

	hudDrawer->Draw((gu->GetMyPlayer())->fpsController.GetControllee());
	debugDrawerAI->Draw();

	backing.DrawInputReceivers();
	backing.DrawInputText();
	backing.DrawInterfaceWidgets();
	RmlGui::RenderFrame();
	mouse->DrawCursor();

	eventHandler.DrawScreenPost();
}
}
