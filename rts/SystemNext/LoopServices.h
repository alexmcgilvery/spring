/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Diagnostics/PhaseToken.h"

namespace runtime {
class IRuntimeMode;
class Session;
class IVisualFrame;

/** Input dispatch preserves the existing route from user intent to commands. */
class ILoopInput {
public:
	virtual ~ILoopInput() = default;
	virtual void ProcessEvents() = 0;
};

/** Lifecycle operations run before dependent session objects are invalidated. */
class ILoopLifecycle {
public:
	virtual ~ILoopLifecycle() = default;
	virtual bool ExitRequested() const = 0;
	virtual bool ReloadRequested() const = 0;
	virtual void ServiceWatchdog() = 0;
	virtual void ProcessQueuedSave() = 0;
	virtual void ReloadSession() = 0;
	virtual void RequestExit() = 0;
};

class ILoopPlatform {
public:
	virtual ~ILoopPlatform() = default;
	virtual void UpdateConfiguration() = 0;
	virtual void UpdateWindow() = 0;
	virtual void UpdateClock() = 0;
};

/** Observation failures must never become failures of engine execution. */
class ILoopDiagnostics {
public:
	virtual ~ILoopDiagnostics() = default;
	virtual PhaseToken BeginPhase(Phase phase) noexcept = 0;
	virtual void EndPhase(PhaseToken token) noexcept = 0;
	virtual void Flush() noexcept = 0;
};

/**
 * Non-owning construction bundle, not an application object.
 * Bindings outlive Run and resolve current session dependencies at call time.
 * Grouping setup does not hide the separate concerns in the execution flow.
 */
struct LoopServices {
	ILoopInput& input;
	ILoopLifecycle& lifecycle;
	ILoopPlatform& platform;
	ILoopDiagnostics& diagnostics;
	IRuntimeMode& mode;
	Session& session;
	IVisualFrame& visuals;
};
}
