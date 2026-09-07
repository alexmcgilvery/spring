/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <string>
#include <vector>

namespace runtime {

// Equality compares the defined IEEE payload, including NaNs and signed zero.
// This prevents padding or repeated NaN comparisons from inventing revisions.
struct SimFloat {
	float value = 0;
	SimFloat() = default;
	SimFloat(float value);
	bool operator==(const SimFloat& other) const noexcept;
};
using SimVector = std::array<SimFloat, 3>;
using SimTransform = std::array<SimFloat, 16>;

enum class EntityKind: std::uint8_t { None, Unit, Feature, SyncedProjectile };
struct EntityKey {
	EntityKind kind = EntityKind::None;
	std::int32_t id = -1;
	std::uint64_t generation = 0;
	std::uint64_t epoch = 0;
	bool operator==(const EntityKey&) const = default;
};

struct PublishedUnit {
	EntityKey key;
	std::int32_t definition = -1;
	std::int32_t model = -1;
	std::int32_t team = -1;
	std::int32_t allyTeam = -1;
	SimVector position, front, right, up, velocity;
	SimFloat health, maxHealth, buildProgress;
	bool beingBuilt = false;
	bool cloaked = false;
	bool dead = false;
	EntityKey transporter;
	SimVector positionError, positionErrorDelta;
	std::array<std::uint32_t, 8> positionErrorMask = {};
	std::array<std::uint8_t, 255> losStatus = {};
	bool operator==(const PublishedUnit&) const = default;
};

struct PublishedFeature {
	EntityKey key;
	std::int32_t definition = -1;
	std::int32_t model = -1;
	std::int32_t team = -1;
	std::int32_t allyTeam = -1;
	SimVector position, front, right, up, velocity;
	SimTransform transform;
	SimFloat health, maxHealth, reclaimLeft;
	bool operator==(const PublishedFeature&) const = default;
};

struct PublishedProjectile {
	EntityKey key, owner;
	std::int32_t team = -1;
	std::int32_t allyTeam = -1;
	std::int32_t weaponDefinition = -1;
	std::int32_t model = -1;
	std::int32_t projectileType = -1;
	SimVector position, direction, velocity;
	bool weapon = false;
	bool piece = false;
	bool hitscan = false;
	bool creationPending = false;
	bool deletionPending = false;
	bool operator==(const PublishedProjectile&) const = default;
};

struct PublishedPiecePose {
	EntityKey entity;
	std::int32_t modelPiece = -1;
	std::int32_t scriptPiece = -1;
	std::int32_t parentPiece = -1;
	SimTransform localTransform;
	bool visible = false;
	std::array<bool, 3> interpolationDiscontinuity = {};
	bool operator==(const PublishedPiecePose&) const = default;
};

struct PublishedGhost {
	EntityKey liveEntity;
	std::int32_t allyTeam = -1;
	std::int32_t team = -1;
	std::int32_t model = -1;
	std::int32_t facing = 0;
	SimVector position, middlePosition, direction;
	SimFloat radius;
	bool dead = false;
	bool operator==(const PublishedGhost&) const = default;
};

struct CatalogPiece {
	std::int32_t model = -1;
	std::int32_t piece = -1;
	std::int32_t parent = -1;
	bool operator==(const CatalogPiece&) const = default;
};

struct CatalogDefinition {
	EntityKind kind = EntityKind::None;
	std::int32_t id = -1;
	std::int32_t model = -1;
	std::pmr::string name;
	explicit CatalogDefinition(std::pmr::memory_resource* memory);
	bool operator==(const CatalogDefinition&) const = default;
};

struct CatalogModel {
	std::int32_t id = -1;
	std::pmr::string name;
	explicit CatalogModel(std::pmr::memory_resource* memory);
	bool operator==(const CatalogModel&) const = default;
};

struct PublishedCatalog {
	std::uint64_t revision = 0;
	std::pmr::vector<CatalogModel> models;
	std::pmr::vector<CatalogDefinition> definitions;
	std::pmr::vector<CatalogPiece> pieces;
	explicit PublishedCatalog(std::pmr::memory_resource* memory);
};

struct PublishedSimFrame {
	std::uint64_t epoch = 0;
	std::uint64_t revision = 0;
	std::int32_t tick = -1;
	std::uint64_t firstEvent = 0;
	std::uint64_t endEvent = 0;
	bool paused = false;
	SimFloat speed, wantedSpeed;
	std::shared_ptr<const PublishedCatalog> catalog;
	std::pmr::vector<PublishedUnit> units;
	std::pmr::vector<PublishedFeature> features;
	std::pmr::vector<PublishedProjectile> projectiles;
	std::pmr::vector<PublishedPiecePose> poses;
	std::pmr::vector<PublishedGhost> ghosts;
	std::pmr::vector<SimFloat> radarErrorSizes;
	explicit PublishedSimFrame(std::pmr::memory_resource* memory);

	bool SameCoveredState(const PublishedSimFrame& other) const;
};

using SimFrameLease = std::shared_ptr<const PublishedSimFrame>;
using CatalogLease = std::shared_ptr<const PublishedCatalog>;

}
