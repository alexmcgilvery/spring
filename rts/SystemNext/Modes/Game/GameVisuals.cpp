/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameVisuals.h"

#include "IGameVisualServices.h"

namespace runtime {
GameVisuals::GameVisuals(IGameVisualServices& services): services(services)
{
}

/** Preserve the draw phase around preparation as well as rendering. */
bool GameVisuals::PrepareAndRender()
{
	auto drawPhase = services.OpenDrawPhase();
	services.SamplePreparationStart();
	if (!PrepareClientState())
		return false;

	services.UpdateDocumentInterface();
	services.SampleDrawStart();
	return RenderFrame();
}

/**
 * Advance client clocks even while skipping. Capture, pause and simulation cadence
 * remain inputs to existing interpolation calculations, not new scheduling policy.
 */
bool GameVisuals::PrepareClientState()
{
	auto presentationScope = services.OpenPresentationScope();
	services.UpdateGameClocksAndSimulationRate();
	if (services.IsSkipping()) {
		if (services.IsSkipScreenDue()) {
			services.RecordSkipScreenTime();
			services.DrawSkipScreen();
		}
		return false;
	}

	const bool newSimulationFrame = services.HasNewSimulationFrame();
	services.AdvanceDrawCountersAndInterpolation();
	services.UpdateDrawRate();
	const bool forceClientUpdate = services.IsClientUpdateDue();
	services.RecordObservedSimulationFrame();
	UpdateClientAndGraphicsState(newSimulationFrame, forceClientUpdate);
	services.ReportPreparationTiming();
	return true;
}

/**
 * Preserve console-before-font and camera-before-upload dependencies. These
 * operations include input submission and Lua/audio effects as well as graphics.
 */
void GameVisuals::UpdateClientAndGraphicsState(bool newSimulationFrame, bool forceClientUpdate)
{
	services.UpdateCameraController();
	services.UpdateLineStippleIconsAndNamedTextures();
	if (newSimulationFrame || forceClientUpdate) {
		services.RecordClientUpdateTime();
		services.UpdateInfoTextureAndSoundListener();
	}
	services.SetNormalDrawMode();
	services.CheckScriptStacksAndActions();
	services.SubmitTextAndLabelInput();
	services.UpdateConsoleAndPublishLines();
	services.UpdateFonts();
	services.UpdateMouseCursorsAndInterface();
	services.DispatchClientUpdate();
	services.UpdateTrackerCameraAndShadows();
	services.UpdateWorldAndUploadTransforms(newSimulationFrame);
	services.UpdateCursorCameraDirection();
	services.UpdateUniformConstants();
}

/**
 * Genesis can generate resources even when the window is inactive. Capture and
 * completion measurements follow all world/UI work and precede ordinary present.
 */
bool GameVisuals::RenderFrame()
{
	auto renderScope = services.OpenRenderScope();
	services.RecordGraphicsStartTimestamp();
	services.SetNormalDrawMode();
	services.BindUniformConstants();
	services.DispatchDrawGenesis();
	if (!services.IsWindowActive()) {
		services.SleepForInactiveWindow();
		if (!services.IsInactiveRefreshDue())
			return false;
	}

	services.CheckInterpolationDiagnostics();
	services.EnableTeamHighlight();
	RenderWorld();
	RenderInterface();
	services.RestoreDepthAndModelView();
	services.CaptureVideoFrameIfEnabled();
	services.SetNotDrawingMode();
	services.DisableTeamHighlight();
	services.RecordDrawCompletionAndTiming();
	return true;
}

/** The minimap never guarantees complete coverage; always prepare/draw the world. */
void GameVisuals::RenderWorld()
{
	services.UpdateMinimap();
	services.GenerateEnvironmentLighting();
	services.RestoreWindowFramebufferAndViewport();
	services.DrawWorld();
	services.ResetWorldMatrices();
}

/** Keep screen-effect, input, document-interface and cursor overlay order visible. */
void GameVisuals::RenderInterface()
{
	auto interfaceScope = services.OpenInterfaceScope();
	services.DrawScreenUnitIcons();
	services.DispatchScreenEffects();
	services.DrawPlayerHudAndAiDebug();
	services.DrawInputReceivers();
	services.DrawInputText();
	services.DrawInterfaceWidgets();
	services.RenderDocumentInterface();
	services.DrawCursor();
	services.DispatchScreenPost();
}

//FIXME GAME-011: CGame::Draw measures avgDrawFrameTime from after RmlGui::Update
// through capture, SetDrawMode and team-highlight disable; it excludes client
// preparation and swap. DbgTimingInfo and GPU end timestamp follow that sample.
// Capture also overwrites globalRendering->lastFrameTime to 1000/GAME_SPEED.
// RecordDrawCompletionAndTiming must preserve assignment order and fresh samples;
// test no completion writes on skipped/inactive/throwing paths and capture timing.

//FIXME GAME-012: No production adapter or mode binding exists. Acquire the visual
// binding again after session update and loading synchronization; do not update a
// replacement twice. The current visual context does not identify which preparation
// ran. Resolve that shared contract before connecting this companion; use fake
// adjacent modes for removal/replacement/cancellation and guard-unwinding tests.
}
