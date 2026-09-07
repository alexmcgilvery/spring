/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyGameServices.h"

//FIXME GAME-BINDING: The copied unqualified CGame members and private helpers
// below deliberately expose unresolved access. They must become narrow backing
// accesses, not copied state or inheritance from CGame. Resolve retirement and
// invocation lifetime before enabling. Missing engine includes are also unresolved.
#if 0
namespace runtime {
// [SESSION: transport, capture pacing, accepted stream, recovery]
//FIXME GAME-SESSION-SCOPE: The current abstract OpenSessionScope factory cannot
// retain RECOIL_DETAILED_TRACY_ZONE past its own return. WithSessionScope below
// retains the exact lexical macro around the supplied session operation body.
// Update the inactive interface and ordering code together before activation.


void LegacyGameServices::WithSessionScope(const std::function<void()>& body)
{
	RECOIL_DETAILED_TRACY_ZONE;
	body();
}

void LegacyGameServices::CheckFloatingPointControl()
{
	good_fpu_control_registers("CGame::Update");
}

void LegacyGameServices::ObserveSessionBootstrap()
{
	runtime::legacy::BeginSession();
}

void LegacyGameServices::ServicePendingJobs()
{
	jobDispatcher.Update();
}

void LegacyGameServices::ServiceTransport()
{
	clientNet->Update();
}

void LegacyGameServices::ServiceCaptureTiming()
{
	// When video recording do step by step simulation, so each simframe gets a corresponding videoframe
	// FIXME: SERVER ALREADY DOES THIS BY ITSELF
	if (playing && gameServer != nullptr && videoCapturing->AllowRecord())
		gameServer->CreateNewFrame(false, true);
}

void LegacyGameServices::EnterSyncedCode()
{
	ENTER_SYNCED_CODE();
}

void LegacyGameServices::SendClientProcessingUsage()
{
	SendClientProcUsage();
}

void LegacyGameServices::ProcessAuthoritativeStream()
{
	//FIXME GAME-STREAM-BINDING: This is the existing authoritative operation, not
	// a new simulation loop. Private CGame access and lifetime are unresolved;
	// bind narrowly without invoking CGame::Update or duplicating SimFrame.
	ClientReadNet(); // issues new SimFrame()s
}

void LegacyGameServices::ServiceConnectionRecovery()
{
	if (!gameOver) {
		if (clientNet->NeedsReconnect())
			clientNet->AttemptReconnect(SpringVersion::GetSync(), Platform::GetPlatformStr());

		if (clientNet->CheckTimeout(0, gs->PreSimFrame()))
			GameEnd({}, true);
	}
}

void LegacyGameServices::LeaveSyncedCode()
{
	LEAVE_SYNCED_CODE();
}

void LegacyGameServices::ReportScriptAllocationFailures()
{
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
}

}
#endif
