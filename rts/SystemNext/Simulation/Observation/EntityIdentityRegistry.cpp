/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EntityIdentityRegistry.h"

namespace runtime {

EntityIdentityRegistry::EntityIdentityRegistry(std::pmr::memory_resource* memory): entries(memory)
{}

void EntityIdentityRegistry::Restart(std::uint64_t newEpoch)
{
	entries.clear();
	epoch = newEpoch;
}

EntityKey EntityIdentityRegistry::Discover(EntityKind kind, std::int32_t id, std::uintptr_t token, bool awaitingCreation)
{
	if (token == 0 || kind == EntityKind::None)
		return {};
	auto& entry = entries[{kind, id}];
	if (!entry.alive || entry.token != token) {
		if (entry.key.generation == std::numeric_limits<std::uint64_t>::max())
			throw std::overflow_error("entity generation exhausted");
		entry.key = {kind, id, entry.key.generation + 1, epoch};
		entry.token = token;
		entry.alive = true;
		entry.dead = false;
		entry.notified = !awaitingCreation;
	}
	return entry.key;
}

EntityKey EntityIdentityRegistry::Created(EntityKind kind, std::int32_t id, std::uintptr_t token)
{
	auto key = Discover(kind, id, token);
	if (key.kind != EntityKind::None)
		entries.at({kind, id}).notified = true;
	return key;
}

EntityKey EntityIdentityRegistry::Resolve(EntityKind kind, std::int32_t id, std::uintptr_t token) const
{
	const auto it = entries.find({kind, id});
	return it != entries.end() && it->second.token == token ? it->second.key : EntityKey{};
}

void EntityIdentityRegistry::SemanticDeath(EntityKey key)
{
	if (auto* entry = Find(key)) entry->dead = true;
}

void EntityIdentityRegistry::Retire(EntityKey key)
{
	if (auto* entry = Find(key)) entry->alive = false;
}

bool EntityIdentityRegistry::Dead(EntityKey key) const
{
	const auto* entry = Find(key);
	return entry != nullptr && entry->dead;
}

bool EntityIdentityRegistry::Alive(EntityKey key) const
{
	const auto* entry = Find(key);
	return entry != nullptr && entry->alive;
}

bool EntityIdentityRegistry::AwaitingCreation(EntityKey key) const
{
	const auto* entry = Find(key);
	return entry != nullptr && !entry->notified;
}

const EntityIdentityRegistry::Entry* EntityIdentityRegistry::Find(EntityKey key) const
{
	const auto it = entries.find({key.kind, key.id});
	return it != entries.end() && it->second.key == key ? &it->second : nullptr;
}

EntityIdentityRegistry::Entry* EntityIdentityRegistry::Find(EntityKey key)
{
	return const_cast<Entry*>(std::as_const(*this).Find(key));
}

}
