/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <map>
#include <string_view>
#include <unordered_map>

#include "SystemNext/Simulation/Observation/EntityIdentityRegistry.h"
#include "SystemNext/Simulation/Observation/SimulationNotifications.h"
#include "SystemNext/Simulation/Publication/SimFramePublicationStore.h"

class CUnit;
class CFeature;
class CProjectile;
class CWeapon;
struct Command;
struct S3DModel;
struct SolidObjectDef;
struct LocalModel;
struct float3;

namespace runtime::legacy {

class LegacySimulationStateReader {
public:
	LegacySimulationStateReader(SimFramePublicationStore& store, EntityIdentityRegistry& identities);
	void Extract(PublishedSimFrame& frame);
	NotificationEntity Copy(const CUnit* unit);
	NotificationEntity Copy(const CFeature* feature);
	NotificationEntity Copy(const CProjectile* projectile);
	NotificationWeapon Copy(const CWeapon* weapon);
	NotificationCommand Copy(const Command& command, std::pmr::memory_resource* memory);
	static SimVector Copy(const float3& vector);
	EntityKey Key(const CUnit* unit);
	EntityKey Key(const CFeature* feature);
	EntityKey Key(const CProjectile* projectile);
	int Model(const S3DModel* model);
	void Reset();

private:
	struct NameHash {
		using is_transparent = void;
		std::size_t operator()(std::string_view value) const { return std::hash<std::string_view>{}(value); }
	};
	int ModelName(std::string_view name);
	void Definition(EntityKind kind, const SolidObjectDef* definition, int model);
	void Pose(PublishedSimFrame& frame, EntityKey entity, const LocalModel& model);
	void Ghosts(PublishedSimFrame& frame);
	CatalogLease FreezeCatalog();
	SimFramePublicationStore& store;
	EntityIdentityRegistry& identities;
	std::pmr::unordered_map<std::pmr::string, int, NameHash, std::equal_to<>> modelNames;
	std::pmr::unordered_map<const S3DModel*, int> loadedModels;
	std::pmr::map<std::pair<EntityKind, int>, std::size_t> definitions;
	PublishedCatalog catalogDraft;
	CatalogLease catalog;
	std::uint64_t catalogRevision = 0;
	bool catalogChanged = false;
};

}
