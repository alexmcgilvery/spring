/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <limits>
#include <map>
#include <memory_resource>
#include <stdexcept>
#include <utility>

#include "SystemNext/Simulation/Publication/PublishedSimFrame.h"

namespace runtime {

// Producer-private lifetime tracking. Tokens can encode legacy addresses, but
// only EntityKey is copied into publications or notifications.
class EntityIdentityRegistry {
public:
	explicit EntityIdentityRegistry(std::pmr::memory_resource* memory);
	void Restart(std::uint64_t newEpoch);

	EntityKey Discover(EntityKind kind, std::int32_t id, std::uintptr_t token, bool awaitingCreation = false);

	EntityKey Created(EntityKind kind, std::int32_t id, std::uintptr_t token);

	EntityKey Resolve(EntityKind kind, std::int32_t id, std::uintptr_t token) const;

	void SemanticDeath(EntityKey key);
	void Retire(EntityKey key);
	bool Dead(EntityKey key) const;
	bool Alive(EntityKey key) const;
	bool AwaitingCreation(EntityKey key) const;

private:
	struct Entry {
		EntityKey key;
		std::uintptr_t token = 0;
		bool alive = false;
		bool dead = false;
		bool notified = false;
	};
	const Entry* Find(EntityKey key) const;
	Entry* Find(EntityKey key);
	std::pmr::map<std::pair<EntityKind, std::int32_t>, Entry> entries;
	std::uint64_t epoch = 0;
};

}
