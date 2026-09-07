/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "LoopServices.h"
#include "Session/SessionUpdate.h"
#include <optional>
#include <cstdint>

namespace runtime {
/**
 * Own application iteration order; initialization and shutdown surround Run.
 * Input -> session -> simulation -> presentation -> rendering -> present are
 * logical responsibilities. Session processing can advance multiple ticks;
 * presentation and rendering still share existing implementations.
 * Visual execution currently blocks further session service. Independent visual
 * scheduling must retain owning inputs and quiesce work before dependency teardown.
 */
class ApplicationLoop {
public:
	explicit ApplicationLoop(LoopServices services);
	void Run();
	void RunIteration();
	/** Keep the host responsive when initial mode setup cannot complete. */
	void BlockCurrentMode(const BlockedFlow& failure);
private:
	void ProcessInputAndLifecycle();
	void ReloadSession();
	void UpdatePlatformState();
	void UpdateAndDraw();
	void UpdateModeBlocks();
	void ApplyApplicationStatus(ApplicationStatus sessionStatus, ApplicationStatus clientStatus);
	void FlushDiagnostics();
	ILoopInput& input;
	ILoopLifecycle& lifecycle;
	ILoopPlatform& platform;
	ILoopDiagnostics& diagnostics;
	Session& session;
	IVisualFrame& visuals;
	ModeBinding& modes;
	std::optional<std::uint64_t> blockedGeneration;
};
}
