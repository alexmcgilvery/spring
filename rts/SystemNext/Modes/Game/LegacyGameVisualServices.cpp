/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyGameVisualServices.h"

//FIXME GAME-BINDING: The copied unqualified CGame members and private helpers
// below deliberately expose unresolved access. They must become narrow backing
// accesses, not copied state or inheritance from CGame. Resolve retirement and
// invocation lifetime before enabling. Missing engine includes are also unresolved.
#if 0
namespace runtime {
//FIXME GAME-SCOPES: Open*Scope in the current abstract interface cannot execute
// these macros in a factory and then return: local timers/GL scopes would already
// have ended. The concrete proposal below uses With*Scope(body) to keep the exact
// lexical lifetime. Change the inactive orchestration contract to scoped callbacks
// (or an equivalent verified scope owner) before binding; never return a dummy
// token. The disabled definitions preserve the real problematic code for review.


bool LegacyGameVisualServices::WithDrawPhase(const std::function<bool()>& body)
{
	runtime::legacy::PhaseScope drawPhase(runtime::Phase::Draw);
	return body();
}

void LegacyGameVisualServices::SamplePreparationStart()
{
	currentTimePreUpdate = spring_gettime();
	currentTime = currentTimePreUpdate;
}

bool LegacyGameVisualServices::WithPresentationScope(const std::function<bool()>& body)
{
	runtime::legacy::PhaseScope presentationPhase(runtime::Phase::Presentation);
	SCOPED_TIMER("Update");
	return body();
}

// [DISPLAY: original clocks and cadence]
void LegacyGameVisualServices::UpdateGameClocksAndSimulationRate()
{
	// timings and frame interpolation
	const spring_time deltaDrawFrameTime = currentTime - globalRendering->lastFrameStart;

	const float modGameDeltaTimeSecs = mix(deltaDrawFrameTime.toMilliSecsf() * 0.001f, 0.01f, skipping);
	unsyncedUpdateDeltaTime = (currentTime - lastUnsyncedUpdateTime).toSecsf();

	{
		// update game timings
		globalRendering->lastFrameStart = currentTime;
		globalRendering->lastFrameTime = deltaDrawFrameTime.toMilliSecsf();

		gu->avgFrameTime = mix(gu->avgFrameTime, deltaDrawFrameTime.toMilliSecsf(), 0.05f);
		gu->gameTime += modGameDeltaTimeSecs;
		gu->modGameTime += (modGameDeltaTimeSecs * gs->speedFactor * (1 - gs->paused));

		totalGameTime += (modGameDeltaTimeSecs * (playing && !gameOver));
		updateDeltaSeconds = modGameDeltaTimeSecs;
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
}

bool LegacyGameVisualServices::IsSkipping() const
{
	return skipping;
}

bool LegacyGameVisualServices::IsSkipScreenDue() const
{
	return !(spring_tomsecs(currentTime - skipLastDrawTime) < 500.0f);
}

void LegacyGameVisualServices::RecordSkipScreenTime()
{
	skipLastDrawTime = currentTime;
}

// [RENDER during DISPLAY]
void LegacyGameVisualServices::DrawSkipScreen()
{
	//FIXME GAME-SKIP-BOUNDARY: This actual renderer currently runs before normal
	// presentation and reports false draw eligibility. Moving it into a later
	// render block could introduce a swap or change 2 Hz behavior. Preserve the
	// early path until display/render result semantics represent this case.
	DrawSkip();
}

bool LegacyGameVisualServices::HasNewSimulationFrame() const
{
	return (lastSimFrame != gs->frameNum);
}

void LegacyGameVisualServices::AdvanceDrawCountersAndInterpolation()
{
	//FIXME GAME-TIMING-BOUNDARY: lastFrameTime and timeOffset are also accessed by
	// SimFrame. The exact smoothing branches below must remain serial with ticks;
	// isolate authoritative versus display timing ownership before decoupling.
	const bool newSimFrame = HasNewSimulationFrame();
	numDrawFrames++;
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
	globalRendering->lastFrameStart = currentTime;
	// Update the interpolation coefficient (globalRendering->timeOffset)
	if (!gs->paused && !IsSimLagging() && !gs->PreSimFrame() && !videoCapturing->AllowRecord()) {
		globalRendering->weightedSpeedFactor = 0.001f * gu->simFPS;
		globalRendering->lastTimeOffset = globalRendering->timeOffset;
		globalRendering->timeOffset = (currentTime - lastFrameTime).toMilliSecsf() * globalRendering->weightedSpeedFactor;

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

		lastSimFrameTime = currentTime;
		lastFrameTime = currentTime;
	}
}

void LegacyGameVisualServices::UpdateDrawRate()
{
	if ((currentTime - frameStartTime).toMilliSecsf() >= 1000.0f) {
		globalRendering->FPS = (numDrawFrames * 1000.0f) / std::max(0.01f, (currentTime - frameStartTime).toMilliSecsf());

		// update draw-FPS counter once every second
		frameStartTime = currentTime;
		numDrawFrames = 0;

	}
}

bool LegacyGameVisualServices::IsClientUpdateDue() const
{
	return (unsyncedUpdateDeltaTime >= INV_GAME_SPEED);
}

void LegacyGameVisualServices::RecordObservedSimulationFrame()
{
	lastSimFrame = gs->frameNum;
}

void LegacyGameVisualServices::UpdateCameraController()
{
	camHandler->UpdateController(playerHandler.Player(gu->myPlayerNum), gu->fpsMode);
}

void LegacyGameVisualServices::UpdateLineStippleIconsAndNamedTextures()
{
	lineDrawer.UpdateLineStipple();

	icon::iconHandler.Update();
	CNamedTextures::Update();
}

void LegacyGameVisualServices::RecordClientUpdateTime()
{
	lastUnsyncedUpdateTime = currentTime;
}

void LegacyGameVisualServices::UpdateInfoTextureAndSoundListener()
{
	infoTextureHandler->Update();
	sound->UpdateListener(camera->GetPos(), camera->GetDir(), camera->GetUp());
}

void LegacyGameVisualServices::SetNormalDrawMode()
{
	SetDrawMode(gameNormalDraw);
}

void LegacyGameVisualServices::CheckScriptStacksAndActions()
{
	if (luaUI != nullptr) {
		luaUI->CheckStack();
		luaUI->CheckAction();
	}
	if (luaGaia != nullptr)
		luaGaia->CheckStack();
	if (luaRules != nullptr)
		luaRules->CheckStack();
}

// [INPUT currently serviced during DISPLAY]
void LegacyGameVisualServices::SubmitTextAndLabelInput()
{
	//FIXME GAME-INPUT-BOUNDARY: These actual command submissions currently follow
	// session processing, interpolation, texture/audio work and Lua stack/actions.
	// An earlier INPUT block would let commands reach a different accepted-input
	// interval and would run during skip paths that currently bypass this code.
	// Keep this body/order while extracting input responsibility; define staged
	// collection versus submission and test pause/skip/Lua-emitted commands before
	// changing the invocation point. Synchronous label handling may read live UI.
	if (gameTextInput.SendPromptInput()) {
		gameConsoleHistory.AddLine(gameTextInput.userInput);
		SendNetChat(gameTextInput.userInput);
		gameTextInput.ClearInput();
	}
	if (inMapDrawer->IsWantLabel() && gameTextInput.SendLabelInput())
		gameTextInput.ClearInput();
}

void LegacyGameVisualServices::UpdateConsoleAndPublishLines()
{
	infoConsole->PushNewLinesToEventHandler();
	infoConsole->Update();
}

void LegacyGameVisualServices::UpdateFonts()
{
	// Console update may have caused font updates.
	CFontTexture::Update();
}

void LegacyGameVisualServices::UpdateMouseCursorsAndInterface()
{
	mouse->Update();
	mouse->UpdateCursors();
	guihandler->Update();
	commandDrawer->Update();
}

void LegacyGameVisualServices::DispatchClientUpdate()
{
	SCOPED_TIMER("Update::EventHandler");
	eventHandler.Update();
}

void LegacyGameVisualServices::UpdateTrackerCameraAndShadows()
{
	if (unitTracker.Enabled())
		unitTracker.SetCam();

	camera->Update();
	shadowHandler.Update();
}

// [DISPLAY crossing into graphics preparation]
void LegacyGameVisualServices::UpdateWorldAndUploadTransforms(bool newSimulationFrame)
{
	//FIXME GAME-UPLOAD-BOUNDARY: These real uploads require graphics/world ownership.
	// A CPU display block cannot move to a worker merely by moving these statements.
	// Keep under the existing context guard until owned render input is available.
	worldDrawer.Update(newSimulationFrame);
	transformsUploader.Update();
	modelUniformsUploader.Update();
}

void LegacyGameVisualServices::UpdateCursorCameraDirection()
{
	mouse->UpdateCursorCameraDir();
}

void LegacyGameVisualServices::UpdateUniformConstants()
{
	UniformConstants::GetInstance().Update();
}

void LegacyGameVisualServices::ReportPreparationTiming()
{
	eventHandler.DbgTimingInfo(TIMING_UNSYNCED, currentTime, spring_now());
}

void LegacyGameVisualServices::UpdateDocumentInterface()
{
	RmlGui::Update();
}

void LegacyGameVisualServices::SampleDrawStart()
{
	currentTimePreDraw = spring_gettime();
}

bool LegacyGameVisualServices::WithRenderScope(const std::function<bool()>& body)
{
	SCOPED_SPECIAL_TIMER("Draw");
	SCOPED_GL_DEBUGGROUP("Draw");
	return body();
}

void LegacyGameVisualServices::RecordGraphicsStartTimestamp()
{
	globalRendering->SetGLTimeStamp(CGlobalRendering::FRAME_REF_TIME_QUERY_IDX);
}

void LegacyGameVisualServices::BindUniformConstants()
{
	UniformConstants::GetInstance().Bind();
}

void LegacyGameVisualServices::DispatchDrawGenesis()
{
	SCOPED_TIMER("Draw::DrawGenesis");
	eventHandler.DrawGenesis();
}

bool LegacyGameVisualServices::IsWindowActive() const
{
	return globalRendering->active;
}

void LegacyGameVisualServices::SleepForInactiveWindow()
{
	spring_sleep(spring_msecs(10));
}

bool LegacyGameVisualServices::IsInactiveRefreshDue() const
{
	//FIXME GAME-INACTIVE-BOUNDARY: This decision follows DrawGenesis and sleep.
	// A render scheduler moving eligibility earlier suppresses actual callbacks;
	// moving it after normal completion changes draw mode and timing assignments.
	return !((currentTimePreDraw - lastDrawFrameTime).toSecsi() < 30);
}

void LegacyGameVisualServices::CheckInterpolationDiagnostics()
{
	if (globalRendering->drawDebug) {
		const float deltaFrameTime = (currentTimePreUpdate - lastSimFrameTime).toMilliSecsf();
		const float deltaNetPacketProcTime  = (currentTimePreUpdate - lastNetPacketProcessTime ).toMilliSecsf();
		const float deltaReceivedPacketTime = (currentTimePreUpdate - lastReceivedNetPacketTime).toMilliSecsf();
		const float deltaSimFramePacketTime = (currentTimePreUpdate - lastSimFrameNetPacketTime).toMilliSecsf();

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
}

void LegacyGameVisualServices::EnableTeamHighlight()
{
	CTeamHighlight::Enable(spring_tomsecs(currentTimePreDraw));
}

void LegacyGameVisualServices::UpdateMinimap()
{
	minimap->Update();
}

void LegacyGameVisualServices::GenerateEnvironmentLighting()
{
	worldDrawer.GenerateIBLTextures();
}

void LegacyGameVisualServices::RestoreWindowFramebufferAndViewport()
{
	if (FBO::IsSupported())
		FBO::Unbind();
	camera->LoadViewport();
}

void LegacyGameVisualServices::DrawWorld()
{
	worldDrawer.Draw();
}

void LegacyGameVisualServices::ResetWorldMatrices()
{
	worldDrawer.ResetMVPMatrices();
}

void LegacyGameVisualServices::WithInterfaceScope(const std::function<void()>& body)
{
	SCOPED_TIMER("Draw::Screen");
	SCOPED_GL_DEBUGGROUP("Draw::Screen");
	body();
}

void LegacyGameVisualServices::DrawScreenUnitIcons()
{
	if (CUnitDrawer::UseScreenIcons())
		unitDrawer->DrawUnitIconsScreen();
}

void LegacyGameVisualServices::DispatchScreenEffects()
{
	eventHandler.DrawScreenEffects();
}

void LegacyGameVisualServices::DrawPlayerHudAndAiDebug()
{
	hudDrawer->Draw((gu->GetMyPlayer())->fpsController.GetControllee());
	debugDrawerAI->Draw();
}

void LegacyGameVisualServices::DrawInputReceivers()
{
	backing.DrawInputReceivers();
}

void LegacyGameVisualServices::DrawInputText()
{
	backing.DrawInputText();
}

void LegacyGameVisualServices::DrawInterfaceWidgets()
{
	backing.DrawInterfaceWidgets();
}

void LegacyGameVisualServices::RenderDocumentInterface()
{
	RmlGui::RenderFrame();
}

void LegacyGameVisualServices::DrawCursor()
{
	mouse->DrawCursor();
}

void LegacyGameVisualServices::DispatchScreenPost()
{
	eventHandler.DrawScreenPost();
}

void LegacyGameVisualServices::RestoreDepthAndModelView()
{
	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();
}

// [RENDER completion: capture precedes ordinary window present]
void LegacyGameVisualServices::CaptureVideoFrameIfEnabled()
{
	if (videoCapturing->AllowRecord()) {
		videoCapturing->SetLastFrameTime(globalRendering->lastFrameTime = 1000.0f / GAME_SPEED);
		// does nothing unless StartCapturing has also been called via /createvideo (Windows-only)
		videoCapturing->RenderFrame();
	}
}

void LegacyGameVisualServices::SetNotDrawingMode()
{
	SetDrawMode(gameNotDrawing);
}

void LegacyGameVisualServices::DisableTeamHighlight()
{
	CTeamHighlight::Disable();
}

void LegacyGameVisualServices::RecordDrawCompletionAndTiming()
{
	const spring_time currentTimePostDraw = spring_gettime();
	const spring_time currentFrameDrawTime = currentTimePostDraw - currentTimePreDraw;
	gu->avgDrawFrameTime = mix(gu->avgDrawFrameTime, currentFrameDrawTime.toMilliSecsf(), 0.05f);

	eventHandler.DbgTimingInfo(TIMING_VIDEO, currentTimePreDraw, currentTimePostDraw);
	globalRendering->SetGLTimeStamp(CGlobalRendering::FRAME_END_TIME_QUERY_IDX);

	lastDrawFrameTime = currentTimePostDraw;
}

}
#endif
