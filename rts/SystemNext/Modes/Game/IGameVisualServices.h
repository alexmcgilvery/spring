/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "IGameServices.h"

namespace runtime {
/**
 * Per-invocation game visual operations with state retained by the backing game.
 * Timing samples/cadence facts belong to an invocation, not a second game owner.
 * This abstract contract is not an immutable snapshot or a worker-safe API.
 * All operations currently require the application thread and current context.
 */
class IGameVisualServices {
public:
	virtual ~IGameVisualServices() = default;
	virtual std::unique_ptr<GameOperationScope> OpenDrawPhase() = 0;
	virtual void SamplePreparationStart() = 0;
	virtual std::unique_ptr<GameOperationScope> OpenPresentationScope() = 0;
	virtual void UpdateGameClocksAndSimulationRate() = 0;
	virtual bool IsSkipping() const = 0;
	virtual bool IsSkipScreenDue() const = 0; // elapsed >= 500 ms
	virtual void RecordSkipScreenTime() = 0;
	virtual void DrawSkipScreen() = 0; // DrawSkip; still rendering during preparation
	virtual bool HasNewSimulationFrame() const = 0;
	virtual void AdvanceDrawCountersAndInterpolation() = 0;
	virtual void UpdateDrawRate() = 0;
	virtual bool IsClientUpdateDue() const = 0; // unsynced delta >= INV_GAME_SPEED
	virtual void RecordObservedSimulationFrame() = 0;
	virtual void UpdateCameraController() = 0;
	virtual void UpdateLineStippleIconsAndNamedTextures() = 0;
	virtual void RecordClientUpdateTime() = 0;
	virtual void UpdateInfoTextureAndSoundListener() = 0;
	virtual void SetNormalDrawMode() = 0;
	virtual void CheckScriptStacksAndActions() = 0;
	virtual void SubmitTextAndLabelInput() = 0;
	virtual void UpdateConsoleAndPublishLines() = 0;
	virtual void UpdateFonts() = 0;
	virtual void UpdateMouseCursorsAndInterface() = 0;
	virtual void DispatchClientUpdate() = 0;
	virtual void UpdateTrackerCameraAndShadows() = 0;
	virtual void UpdateWorldAndUploadTransforms(bool newSimulationFrame) = 0;
	virtual void UpdateCursorCameraDirection() = 0;
	virtual void UpdateUniformConstants() = 0;
	virtual void ReportPreparationTiming() = 0;
	virtual void UpdateDocumentInterface() = 0; // RmlGui::Update
	virtual void SampleDrawStart() = 0;
	virtual std::unique_ptr<GameOperationScope> OpenRenderScope() = 0;
	virtual void RecordGraphicsStartTimestamp() = 0;
	virtual void BindUniformConstants() = 0;
	virtual void DispatchDrawGenesis() = 0;
	virtual bool IsWindowActive() const = 0;
	virtual void SleepForInactiveWindow() = 0; // 10 ms
	virtual bool IsInactiveRefreshDue() const = 0; // sampled pre-draw time; >= 30 whole seconds
	virtual void CheckInterpolationDiagnostics() = 0;
	virtual void EnableTeamHighlight() = 0;
	virtual void UpdateMinimap() = 0;
	virtual void GenerateEnvironmentLighting() = 0;
	virtual void RestoreWindowFramebufferAndViewport() = 0;
	virtual void DrawWorld() = 0;
	virtual void ResetWorldMatrices() = 0;
	virtual std::unique_ptr<GameOperationScope> OpenInterfaceScope() = 0;
	virtual void DrawScreenUnitIcons() = 0;
	virtual void DispatchScreenEffects() = 0;
	virtual void DrawPlayerHudAndAiDebug() = 0;
	virtual void DrawInputReceivers() = 0;
	virtual void DrawInputText() = 0;
	virtual void DrawInterfaceWidgets() = 0;
	virtual void RenderDocumentInterface() = 0;
	virtual void DrawCursor() = 0;
	virtual void DispatchScreenPost() = 0;
	virtual void RestoreDepthAndModelView() = 0;
	virtual void CaptureVideoFrameIfEnabled() = 0;
	virtual void SetNotDrawingMode() = 0;
	virtual void DisableTeamHighlight() = 0;
	virtual void RecordDrawCompletionAndTiming() = 0;
};

//FIXME GAME-006: SamplePreparationStart/SampleDrawStart describe existing
// spring_gettime samples; CGame also uses spring_now for debug timing. The adapter
// has no approved invocation storage yet. Choose an explicit invocation owner so
// nested Lua execution cannot overwrite samples. Keep original clock precision,
// static sim-FPS/debug history lifetime, and averages' exact intervals. Tests need
// a deterministic clock and skipped/inactive/exception traces before binding.

//FIXME GAME-007: UpdateUnsynced mutates gu clocks, CGame timing, render counters,
// interpolation, Lua actions, chat/label submission, sound and GPU uploads. It is
// not read-only preparation. SimFrame reads/writes lastFrameTime and timeOffset
// for smoothing. Do not schedule these services concurrently with simulation.
// Inventory each shared field and synchronous Lua/live-world dependency, then
// establish publication/projection ownership before independent rendering.

//FIXME GAME-008: CGame::Draw sets draw mode, binds uniforms and dispatches
// DrawGenesis before the inactive-window early return. That return does not run
// SetNotDrawingMode, capture or completion timing. Profiler/GL debug scopes still
// unwind. Preserve this distinction; adding a generic finally cleanup changes
// state visible to the next iteration. Test suppressed and forced inactive draws.

//FIXME GAME-009: UpdateUnsynced's skip branch may DrawSkip yet CGame::Draw returns
// false, suppressing ordinary swap eligibility. Its 2 Hz cadence bypasses draw
// counters and every later client operation. Resolve any policy change separately;
// a generic preparation-success flag must not erase this graphics side effect.

//FIXME GAME-010: World/interface callbacks use live Lua, cameras, entities, fonts,
// FBOs and GL state. DrawInputReceivers owns manual Lua-last ordering and a hidden
// interface dual-screen path. Adapters must reuse those operations with their
// nested timer/GL scopes, not flatten callbacks into a new renderer. HEADLESS and
// legacy variants plus hide-interface, dual-screen and throwing Lua fixtures gate
// extraction. The outer loading guard must span this companion through swap.
}
