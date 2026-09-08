/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../../../Snapshots/InvocationMetadata.h"

#include <cstdint>

namespace runtime {

// Forward declarations: notification input and owning observation journal.
struct SimulationNotification;
struct SimulationObservations;

struct SimulationObservationContext {
public:
	const InvocationMetadata& invocation;
	const SimulationNotification& notification;
	SimulationObservations& output;
	std::uint64_t sequence;
};

} // namespace runtime
