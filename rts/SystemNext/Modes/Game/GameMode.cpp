/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameMode.h"

#include "IGameServices.h"

namespace runtime {
bool GameMode::HandlesSession() const
{
	return true;
}

/**
 * Keep job/transport progress ahead of authoritative processing. Allocation
 * failures report through the existing mechanism after leaving synced code;
 * global exit remains monotonic even though this update traditionally returns true.
 */
SessionUpdate GameMode::UpdateSession(Session&)
{
	auto& services = GameServices();
	auto updateScope = services.OpenSessionScope();
	services.CheckFloatingPointControl();
	services.ObserveSessionBootstrap();

	ServiceSession(services);
	services.ReportScriptAllocationFailures();
	return SessionUpdate::FromContinuation(true);
}

/**
 * Capture pacing can request a server frame but cannot bypass accepted network
 * order. Keep usage, message budget and connection recovery in one synced region.
 */
void GameMode::ServiceSession(IGameServices& services)
{
	services.ServicePendingJobs();
	services.ServiceTransport();
	services.ServiceCaptureTiming();

	services.EnterSyncedCode();
	services.SendClientProcessingUsage();
	services.ProcessAuthoritativeStream();
	services.ServiceConnectionRecovery();
	services.LeaveSyncedCode();
}

//FIXME GAME-001: Session::Advance currently invokes the whole active controller
// through LegacySession. Calling it here would recurse after binding GameMode.
// CGame::ClientReadNet already owns packet budgets, command/tick interleaving and
// SimFrame calls. Bind ProcessAuthoritativeStream to that operation, then replace
// the generic Session bridge without adding a second pump. Require accepted-input
// ordering and per-tick checksum parity before this abstract mode is activated.

//FIXME GAME-002: CGame::Update uses explicit ENTER/LEAVE_SYNCED_CODE, not an
// exception-restoring sync guard. GameEnd dispatches synchronous GameOver clients
// and jobs may execute arbitrary callbacks. Resolve backing retirement policy and
// exception traces before adapter reads span these callbacks; do not silently
// add cleanup or suppress exceptions. Service object survival alone cannot prove
// CGame/Lua/network survival. Preserve GameEndOnConnectionLoss's local fallback.

//FIXME GAME-003: ServiceCaptureTiming must preserve playing/server/AllowRecord
// eligibility and CreateNewFrame(false, true). Existing source itself questions
// duplicate server pacing. Treat that as a separate decision, not permission to
// remove the request; compare recording tick/frame pairs and disabled recording.
}
