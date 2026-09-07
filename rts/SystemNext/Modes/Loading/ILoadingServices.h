/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>
#include <string_view>

namespace runtime {
/** Hold the existing recursive progress lock across synchronous progress work. */
class LoadingProgressScope {
public:
	virtual ~LoadingProgressScope();
};

/**
 * Private extraction boundary; no implementation is bound to the engine yet.
 * The existing loading controller owns queued messages, timing and threads.
 * Implementations must resolve that owner at each invocation and must not copy
 * its state into a second owner. Mode/service objects outlive its retirement.
 * Existing exceptions propagate; teardown ordering is not exception suppression.
 */
class ILoadingServices {
public:
	virtual ~ILoadingServices();

	// Delivery retains the recursive lock, checks FPU per message, invokes the
	// intro only when present and clears messages only in that same branch.
	virtual void DeliverProgressNotifications() = 0;
	virtual bool IsGameLoadingComplete() const = 0;
	virtual bool IsMultithreadedLoading() const = 0;
	virtual void KeepWindowResponsive() = 0;

	// Retirement is terminal for backing access. It includes Kill/DeleteInstance:
	// intro shutdown/free, loader join, lock/context restoration, heartbeat stop
	// and join, then the destructor's conditional game activation.
	virtual void RetireLoadingController() = 0;
	// Independent of retired storage; preserves FinishedLoading's quit check,
	// player/path messages, cursor restoration and conditional sound setup.
	virtual void AnnounceLoadingCompletion() = 0;

	// Acquire after KeepWindowResponsive. Enqueue owned text/replaceLast, perform
	// existing log/cleanup and FPU check, and retain the lock in the returned scope.
	// A successful call must return a non-null scope; exceptions release the lock.
	virtual std::unique_ptr<LoadingProgressScope> QueueProgressNotification(std::string_view text, bool replaceLast) = 0;

	// Preserve the 50 FPS MT calculation, unsigned millisecond conversion and
	// lastDrawTime = pre-sleep now. ST does not read/write this pacing state.
	virtual void PaceLoadingFrame() = 0;
	virtual void AdvanceDrawCounter() = 0;
	virtual void MaintainLobbyConnection() = 0;
	virtual bool HasLoadingIntro() const = 0;
	virtual void UpdateLoadingIntro() = 0;
	virtual void DrawLoadingGenesis() = 0;
	virtual void ClearLoadingScreen() = 0;
	virtual void DrawLoadingScreen() = 0;
	virtual void PresentSynchronousLoadingFrame() = 0;
};

//FIXME [LOAD-001] CLoadScreen::SetLoadMessage holds mutex across Update/Draw;
// Update iterates loadMessages while LuaIntro::LoadProgress may execute script.
// Recursive queue insertion could invalidate that iteration, and completion
// could destroy a still-locked backing mutex. Establish reachable callback paths
// and a retirement/queue policy before implementing these services; test nested
// notifications, completion during progress and exceptions with lock lifetimes.
// Do not silently replace iteration with a drained copy: that changes ordering.
//
//FIXME [LOAD-002] CLoadScreen::Kill, its destructor and FinishedLoading split
// teardown across multiple owners. Cancellation/failed initialization can leave
// different subsets initialized; Kill also has an MT/non-joinable early return.
// Map every Init failure and globalQuit branch before decomposing retirement.
// Test intro shutdown, both joins, lock reset, activation and completion sends
// with fake GameMode; no old backing access is permitted after retirement.
//
//FIXME [LOAD-003] CLoadScreen::Init comments and GameLoadThread.h describe hidden
// context creation, but CGameLoadThread::WrapFunc currently sets thread/FPU state
// and invokes loading without that operation. CLoadLock supplies separate context
// binding when thread safety is enabled; its disabled implementation is a no-op.
// Trace actual context ownership and the opengl_error fallback, rather than infer
// it from comments. Require ST/MT/headless initialization and failure evidence
// before extracting startup or adding a guard to the synchronous progress path.
}
