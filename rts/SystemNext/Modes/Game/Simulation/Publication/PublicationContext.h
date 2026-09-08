/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "PublishedFrame.h"
#include "../../../../Globals/InvocationContext.h"

#include <cstdint>
#include <optional>

namespace runtime {

// Forward declarations: authoritative input and owning publication storage.
struct GameSimulationState;
struct SimulationObservations;

struct PublicationContext {
public:
	const InvocationContext& invocation;
	const GameSimulationState& simulation;
	const SimulationObservations& observations;
	PublishedFrameLease& output;
	int completedTick;
	std::optional<std::uint32_t> cachedChecksum;
};

} // namespace runtime
