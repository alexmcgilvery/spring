/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/LoopServices.h"
#include "SystemNext/Session/LegacySession.h"
#include "SystemNext/Presentation/LegacyVisualFrame.h"

class SpringApp;

namespace runtime::legacy {
/**
 * Construct all loop bindings once, without touching engine globals.
 * Operations resolve current dependencies each time, so bindings survive reload
 * and can also exist after failed initialization without dereferencing them.
 */
class LegacyLoopServices final : public ILoopInput, public ILoopLifecycle, public ILoopPlatform, public ILoopDiagnostics {
public:
	explicit LegacyLoopServices(SpringApp& host);
	LoopServices Bind();
	void ProcessEvents() override;
	bool ExitRequested() const override;
	bool ReloadRequested() const override;
	void ServiceWatchdog() override;
	void ProcessQueuedSave() override;
	void ReloadSession() override;
	void RequestExit() override;
	void UpdateConfiguration() override;
	void UpdateWindow() override;
	void UpdateClock() override;
	PhaseToken BeginPhase(Phase phase) noexcept override;
	void EndPhase(PhaseToken token) noexcept override;
	void Flush() noexcept override;
private:
	SpringApp& host;
	LegacyRuntimeMode mode;
	LegacySession session;
	LegacyVisualFrame visuals;
};
}
