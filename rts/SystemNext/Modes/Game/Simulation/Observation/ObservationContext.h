/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../../../Globals/InvocationContext.h"
#include <cstdint>
namespace runtime {
struct SimulationNotification;
struct SimulationObservations;
struct SimulationObservationContext {
	const InvocationContext& invocation;
	const SimulationNotification& notification;
	SimulationObservations& output;
	std::uint64_t sequence;
};
/* Notification is an event-time borrowed input. Any retained payload must be
 * copied into output before nested callbacks can mutate or retire the source.
 * No world/command authority is exposed. Payload and journal definitions follow
 * annotation; there is no event dispatcher or retention implementation here. */
}
