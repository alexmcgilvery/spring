/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

//FIXME GAME-BLOCKED: Invocation storage, private access and lexical scope contract remain unresolved.
// This entire proposed adapter is disabled and NOT typechecked. Original engine
// statements are retained below; production still uses CGame. No fake fallback.
#if 0
#include <functional>
#include "System/Misc/SpringTime.h"

namespace runtime {
class LegacyGameVisualServices {
public:
	bool WithDrawPhase(const std::function<bool()>& body);
	void SamplePreparationStart();
	bool WithPresentationScope(const std::function<bool()>& body);
	void UpdateGameClocksAndSimulationRate();
	bool IsSkipping() const;
	bool IsSkipScreenDue() const;
	void RecordSkipScreenTime();
	void DrawSkipScreen();
	bool HasNewSimulationFrame() const;
	void AdvanceDrawCountersAndInterpolation();
	void UpdateDrawRate();
	bool IsClientUpdateDue() const;
	void RecordObservedSimulationFrame();
	void UpdateCameraController();
	void UpdateLineStippleIconsAndNamedTextures();
	void RecordClientUpdateTime();
	void UpdateInfoTextureAndSoundListener();
	void SetNormalDrawMode();
	void CheckScriptStacksAndActions();
	void SubmitTextAndLabelInput();
	void UpdateConsoleAndPublishLines();
	void UpdateFonts();
	void UpdateMouseCursorsAndInterface();
	void DispatchClientUpdate();
	void UpdateTrackerCameraAndShadows();
	void UpdateWorldAndUploadTransforms(bool newSimulationFrame);
	void UpdateCursorCameraDirection();
	void UpdateUniformConstants();
	void ReportPreparationTiming();
	void UpdateDocumentInterface();
	void SampleDrawStart();
	bool WithRenderScope(const std::function<bool()>& body);
	void RecordGraphicsStartTimestamp();
	void BindUniformConstants();
	void DispatchDrawGenesis();
	bool IsWindowActive() const;
	void SleepForInactiveWindow();
	bool IsInactiveRefreshDue() const;
	void CheckInterpolationDiagnostics();
	void EnableTeamHighlight();
	void UpdateMinimap();
	void GenerateEnvironmentLighting();
	void RestoreWindowFramebufferAndViewport();
	void DrawWorld();
	void ResetWorldMatrices();
	void WithInterfaceScope(const std::function<void()>& body);
	void DrawScreenUnitIcons();
	void DispatchScreenEffects();
	void DrawPlayerHudAndAiDebug();
	void DrawInputReceivers();
	void DrawInputText();
	void DrawInterfaceWidgets();
	void RenderDocumentInterface();
	void DrawCursor();
	void DispatchScreenPost();
	void RestoreDepthAndModelView();
	void CaptureVideoFrameIfEnabled();
	void SetNotDrawingMode();
	void DisableTeamHighlight();
	void RecordDrawCompletionAndTiming();
private:
	//FIXME GAME-INVOCATION: These replace locals shared across extracted operations,
	// not CGame state. Decide an invocation-owned adapter with no nested overwrite.
	// currentTime aliases the original pre-update sample; all other game fields in
	// the source remain unresolved backing accesses, deliberately not copied here.
	spring_time currentTimePreUpdate;
	spring_time currentTimePreDraw;
	spring_time currentTime;
	float unsyncedUpdateDeltaTime;
};
}
#endif
