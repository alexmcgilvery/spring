/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>

namespace runtime {
/** A lexical profiler/diagnostic scope; no gameplay cleanup belongs here. */
class GameOperationScope {
public:
	virtual ~GameOperationScope();
};

/**
 * Private compatibility boundary for session operations, never controller aliases.
 * Proposed concrete bodies are disabled in LegacyGameServices.cpp; they are not bindings. Implementations must retain backing
 * ownership in CGame and resolve it at safe invocation boundaries. Engine work
 * exceptions propagate; observation-only failures use the observation policy.
 */
class IGameServices {
public:
	virtual ~IGameServices() = default;
	virtual std::unique_ptr<GameOperationScope> OpenSessionScope() = 0;
	virtual void CheckFloatingPointControl() = 0;
	virtual void ObserveSessionBootstrap() = 0;
	virtual void ServicePendingJobs() = 0; // jobDispatcher.Update
	virtual void ServiceTransport() = 0; // clientNet->Update
	virtual void ServiceCaptureTiming() = 0;
	virtual void EnterSyncedCode() = 0;
	virtual void SendClientProcessingUsage() = 0; // SendClientProcUsage
	virtual void ProcessAuthoritativeStream() = 0; // ClientReadNet, including SimFrame
	virtual void ServiceConnectionRecovery() = 0; // !gameOver, reconnect then timeout
	virtual void LeaveSyncedCode() = 0;
	virtual void ReportScriptAllocationFailures() = 0;
};

//FIXME GAME-004: NETMSG_NEWFRAME observes completion only after sync response,
// cached previous checksum, optional demo checksums, 4096-frame reset and traffic
// accounting. SimFrame/GameFramePost is earlier and cannot replace this boundary.
// Preserve the pre-reset cached checksum per tick, including multiple ticks in
// one pump; end-of-update observation cannot reconstruct intermediate frames.
// Adapter tests must cover valid/invalid keyframes and checksum-disabled builds.

//FIXME GAME-005: CGame::Update's allocation-error loop mutates msgBuf and logs
// error.msgBuf for each newline; it ORs synced OOM into globalQuit. A cleaner
// parser would change observable messages. Reuse the body once, document any
// separate bug fix, and test multiple lines, missing newline and existing quit.
}
