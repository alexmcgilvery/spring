/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

//FIXME GAME-BLOCKED: Private CGame access and scope callback integration are unresolved.
// This entire proposed adapter is disabled and NOT typechecked. Original engine
// statements are retained below; production still uses CGame. No fake fallback.
#if 0
#include <functional>
#include "System/Misc/SpringTime.h"

namespace runtime {
class LegacyGameServices {
public:
	void WithSessionScope(const std::function<void()>& body);
	void CheckFloatingPointControl();
	void ObserveSessionBootstrap();
	void ServicePendingJobs();
	void ServiceTransport();
	void ServiceCaptureTiming();
	void EnterSyncedCode();
	void SendClientProcessingUsage();
	void ProcessAuthoritativeStream();
	void ServiceConnectionRecovery();
	void LeaveSyncedCode();
	void ReportScriptAllocationFailures();
};
}
#endif
