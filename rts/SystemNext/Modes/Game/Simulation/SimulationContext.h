/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../../Snapshots/InvocationMetadata.h"

namespace runtime {

// Forward declarations: authoritative GameMode ownership.
struct GameSimulationState;
struct SimulationEvents;

/** Authority supplied only by accepted GameMode session work. */
struct SimulationStepContext {
public:
	const InvocationMetadata& invocation;
	GameSimulationState& simulation;
	SimulationEvents& events;
	int acceptedTick;
};

} // namespace runtime
