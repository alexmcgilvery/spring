/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>
#include <variant>
#include <vector>

#include "SystemNext/Simulation/Publication/PublishedSimFrame.h"

namespace runtime {

// Event-time values, copied before later clients can dispatch nested callbacks.
struct NotificationEntity {
	EntityKey key;
	std::int32_t definition = -1;
	std::int32_t model = -1;
	std::int32_t team = -1;
	std::int32_t allyTeam = -1;
	SimVector position;
	SimFloat health, experience, buildProgress;
	bool dead = false;
	bool cloaked = false;
};

struct NotificationWeapon {
	EntityKey owner;
	std::int32_t definition = -1;
	std::int32_t index = -1;
	std::int32_t stockpiled = 0;
};

struct NotificationCommand {
	std::int32_t id = 0;
	std::int32_t timeout = 0;
	std::uint32_t tag = 0;
	std::uint8_t options = 0;
	std::pmr::vector<SimFloat> parameters;
};

struct GameStartNotification {
};

struct GameOverNotification {
	std::pmr::vector<std::uint8_t> winningAllyTeams;
};

struct GamePausedNotification {
	std::int32_t playerID;
	bool paused;
};

struct TeamDiedNotification {
	std::int32_t teamID;
};

struct TeamChangedNotification {
	std::int32_t teamID;
};

struct PlayerChangedNotification {
	std::int32_t playerID;
};

struct PlayerAddedNotification {
	std::int32_t playerID;
};

struct PlayerRemovedNotification {
	std::int32_t playerID;
	std::int32_t reason;
};

struct UnitCreatedNotification {
	NotificationEntity unit;
	NotificationEntity builder;
};

struct UnitFinishedNotification {
	NotificationEntity unit;
};

struct UnitFromFactoryNotification {
	NotificationEntity unit;
	NotificationEntity factory;
	bool userOrders;
};

struct UnitReverseBuiltNotification {
	NotificationEntity unit;
};

struct UnitConstructionDecayedNotification {
	NotificationEntity unit;
	SimFloat timeSinceLastBuild;
	SimFloat iterationPeriod;
	SimFloat part;
};

struct UnitDestroyedNotification {
	NotificationEntity unit;
	NotificationEntity attacker;
	std::int32_t weaponDefID;
};

struct UnitTakenNotification {
	NotificationEntity unit;
	std::int32_t oldTeam;
	std::int32_t newTeam;
};

struct UnitGivenNotification {
	NotificationEntity unit;
	std::int32_t oldTeam;
	std::int32_t newTeam;
};

struct UnitIdleNotification {
	NotificationEntity unit;
};

struct UnitCommandNotification {
	NotificationEntity unit;
	NotificationCommand command;
	std::int32_t playerNum;
	bool fromSynced;
	bool fromLua;
};

struct UnitCmdDoneNotification {
	NotificationEntity unit;
	NotificationCommand command;
};

struct UnitDamagedNotification {
	NotificationEntity unit;
	NotificationEntity attacker;
	SimFloat damage;
	std::int32_t weaponDefID;
	std::int32_t projectileID;
	bool paralyzer;
};

struct UnitStunnedNotification {
	NotificationEntity unit;
	bool stunned;
};

struct UnitExperienceNotification {
	NotificationEntity unit;
	SimFloat oldExperience;
};

struct UnitHarvestStorageFullNotification {
	NotificationEntity unit;
};

struct UnitSeismicPingNotification {
	NotificationEntity unit;
	std::int32_t allyTeam;
	SimVector pos;
	SimFloat strength;
};

struct UnitEnteredRadarNotification {
	NotificationEntity unit;
	std::int32_t allyTeam;
};

struct UnitEnteredLosNotification {
	NotificationEntity unit;
	std::int32_t allyTeam;
};

struct UnitLeftRadarNotification {
	NotificationEntity unit;
	std::int32_t allyTeam;
};

struct UnitLeftLosNotification {
	NotificationEntity unit;
	std::int32_t allyTeam;
};

struct UnitEnteredUnderwaterNotification {
	NotificationEntity unit;
};

struct UnitEnteredWaterNotification {
	NotificationEntity unit;
};

struct UnitEnteredAirNotification {
	NotificationEntity unit;
};

struct UnitLeftUnderwaterNotification {
	NotificationEntity unit;
};

struct UnitLeftWaterNotification {
	NotificationEntity unit;
};

struct UnitLeftAirNotification {
	NotificationEntity unit;
};

struct UnitLoadedNotification {
	NotificationEntity unit;
	NotificationEntity transport;
};

struct UnitUnloadedNotification {
	NotificationEntity unit;
	NotificationEntity transport;
};

struct UnitCloakedNotification {
	NotificationEntity unit;
};

struct UnitDecloakedNotification {
	NotificationEntity unit;
};

struct UnitMovedNotification {
	NotificationEntity unit;
};

struct UnitMoveFailedNotification {
	NotificationEntity unit;
};

struct UnitArrivedAtGoalNotification {
	NotificationEntity unit;
};

struct FeatureCreatedNotification {
	NotificationEntity feature;
};

struct FeatureDestroyedNotification {
	NotificationEntity feature;
};

struct FeatureDamagedNotification {
	NotificationEntity feature;
	NotificationEntity attacker;
	SimFloat damage;
	std::int32_t weaponDefID;
	std::int32_t projectileID;
};

struct FeatureMovedNotification {
	NotificationEntity feature;
	SimVector oldpos;
};

struct ProjectileCreatedNotification {
	NotificationEntity proj;
};

struct ProjectileDestroyedNotification {
	NotificationEntity proj;
};

struct StockpileChangedNotification {
	NotificationEntity unit;
	NotificationWeapon weapon;
	std::int32_t oldCount;
};

struct RenderUnitDestroyedNotification {
	NotificationEntity unit;
};

using NotificationPayload = std::variant<
	GameStartNotification,
	GameOverNotification,
	GamePausedNotification,
	TeamDiedNotification,
	TeamChangedNotification,
	PlayerChangedNotification,
	PlayerAddedNotification,
	PlayerRemovedNotification,
	UnitCreatedNotification,
	UnitFinishedNotification,
	UnitFromFactoryNotification,
	UnitReverseBuiltNotification,
	UnitConstructionDecayedNotification,
	UnitDestroyedNotification,
	UnitTakenNotification,
	UnitGivenNotification,
	UnitIdleNotification,
	UnitCommandNotification,
	UnitCmdDoneNotification,
	UnitDamagedNotification,
	UnitStunnedNotification,
	UnitExperienceNotification,
	UnitHarvestStorageFullNotification,
	UnitSeismicPingNotification,
	UnitEnteredRadarNotification,
	UnitEnteredLosNotification,
	UnitLeftRadarNotification,
	UnitLeftLosNotification,
	UnitEnteredUnderwaterNotification,
	UnitEnteredWaterNotification,
	UnitEnteredAirNotification,
	UnitLeftUnderwaterNotification,
	UnitLeftWaterNotification,
	UnitLeftAirNotification,
	UnitLoadedNotification,
	UnitUnloadedNotification,
	UnitCloakedNotification,
	UnitDecloakedNotification,
	UnitMovedNotification,
	UnitMoveFailedNotification,
	UnitArrivedAtGoalNotification,
	FeatureCreatedNotification,
	FeatureDestroyedNotification,
	FeatureDamagedNotification,
	FeatureMovedNotification,
	ProjectileCreatedNotification,
	ProjectileDestroyedNotification,
	StockpileChangedNotification,
	RenderUnitDestroyedNotification
>;

std::string_view NotificationName(const NotificationPayload& payload);

}
