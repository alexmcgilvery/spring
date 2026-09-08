/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Snapshots/SnapshotTypes.h"

namespace runtime {

// Forward declarations: lifecycle constructs and owns concrete mode resources.
class IMode;

/**
 * Platform collection and lifecycle surround mode execution.
 *
 * Why: OS exit, initialization failure and reload remain actionable even without
 * a mode or visual output. Normal menu/game decisions arrive through Session.
 * CreateMode prepares a replacement from owning handoff data; the loop commits
 * activation only after construction succeeds. Returning null means failure.
 * Expected source: [SpringApp.cpp](../../../System/SpringApp.cpp),
 * SpringApp::Run(), Init(), Reload(), Kill(), MainEventHandler().
 * Resource ownership, platform filtering and production adaptation remain work.
 */
class ApplicationHost {
public:
	virtual ~ApplicationHost();

	virtual void Initialize() = 0;
	virtual bool BeginIteration() = 0;
	virtual ApplicationSnapshot CollectInput() = 0;
	virtual IterationTiming CaptureVisualTiming() = 0;

	virtual bool ExitRequested() const = 0;
	virtual bool ReloadRequested() const = 0;
	virtual std::shared_ptr<IMode> CreateMode(const LifecycleRequest& request) = 0;

	virtual void FlushDiagnostics() = 0;
	virtual void Shutdown() = 0;
};

} // namespace runtime
