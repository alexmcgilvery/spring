/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../../../Globals/InvocationContext.h"
#include "PublishedFrame.h"
#include <cstdint>
#include <optional>
namespace runtime {
struct GameWorldView;
struct PublicationStorage;
struct SimulationObservations;
struct PublicationContext {
	const InvocationContext& invocation;
	const GameWorldView& world;
	const SimulationObservations& observations;
	PublicationStorage& storage;
	PublishedFrameLease& output;
	int completedTick;
	std::optional<std::uint32_t> cachedChecksum;
};
/* Capture cachedChecksum at the completed-frame caller boundary. nullopt means
 * checksum support/data is absent, not zero. Extraction reads world and writes
 * only owned publication storage/output. Deep non-mutation still needs validation.
 * Storage, observations and frame schema are undeveloped contracts, not empty data. */
}
