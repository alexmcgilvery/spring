/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>

namespace runtime {
/**
 * Describe the existing menu operations without acquiring engine ownership.
 *
 * This unbound service contract is intentionally abstract. Each implementation
 * must reuse the named operation, not invoke the entire controller Update/Draw.
 * Services outlive the mode and visuals; they must not retain a Lua handler or
 * controller reference across a callback. All calls are synchronous on the
 * current application thread. Exceptions propagate to the existing host handler.
 */
class ILuaMenuServices {
public:
	virtual ~ILuaMenuServices() = default;

	// Entry assertion from CLuaMenuController::{Update,Draw}; not a fallback.
	virtual void RequireActiveLuaHandler() = 0;

	// CEventHandler::CollectGarbage(false), then InfoConsole's queued lines.
	virtual void CollectScriptGarbage() = 0;
	virtual void PublishConsoleLines() = 0;
	// CMouseHandler::{Update,UpdateCursors}, in that order.
	virtual void UpdateMouse() = 0;
	virtual void UpdateCursors() = 0;
	// Dispatch through CEventHandler, preserving every subscribed client.
	virtual void DispatchClientUpdate() = 0;
	// CMouseHandler::GetCurrentTooltip invokes IsAbove after Update callbacks.
	virtual void EvaluateTooltip() = 0;

	virtual bool IsWindowActive() const = 0;
	// CLuaMenu::AllowDraw preserves its true default for absent/failed call-ins.
	virtual bool RequestScriptDrawPermission() = 0;
	// Sample spring_gettime here; preserve spring_time::toSecsi truncation.
	virtual std::int64_t WholeSecondsSinceLastCompletedDraw() = 0;
	virtual void SleepForSkippedDraw() = 0; // spring_msecs(10).sleep(true)

	// Preserve max(1U, drawFrame + 1), including unsigned wrap behavior.
	virtual void AdvanceDrawCounter() = 0;
	// Reuse Rendering/GL/myGL.cpp ClearScreen, including its buffer clearing.
	virtual void ClearScreen() = 0;
	// Keep EventHandler dispatch: it enables/resets/disables Lua drawing state.
	virtual void DispatchDrawGenesis() = 0;
	virtual void DispatchDrawScreen() = 0;
	virtual void DrawCursor() = 0;
	virtual void DispatchDrawScreenPost() = 0;
	// Sample a fresh time after all drawing succeeds, not on entry or present.
	virtual void RecordCompletedDrawTime() = 0;
};

//FIXME [LUA-OWNERSHIP] CLuaMenuController owns menuArchive/lastDrawFrameTime,
// while CLuaMenu::{LoadFreeHandler,FreeHandler} owns the separate global handler.
// SpringApp::Reload keeps that handler (PersistOnReload) and calls Reset on the
// surviving controller. Activation/reset do not reset its draw timestamp. Before
// binding services, choose a stable owner for that timestamp and archive identity
// and prove reset/reactivation retain them while reconstruction initializes time.
// Do not cache luaMenu across callbacks or silently reconstruct it on activation.

//FIXME [LUA-ARCHIVES] CLuaMenuController::Reset uses GrabLock/FreeLock around
// VFS archive/name changes, then ReloadCursors. Invalid archives take ResetCursor
// instead; Activate separately refuses a null handler and sets activeController
// before ShowMouse/ActivateMenu. These are distinct lifecycle operations, not an
// EnterMode alias. Extract them with failed-load/reset/activation fixtures before
// registration; audit exceptions during AddArchiveWithDeps without accidentally
// changing archive-lock cleanup policy as part of the orchestration extraction.
}
