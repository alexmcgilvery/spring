/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimulationNotifications.h"

namespace runtime {

std::string_view NotificationName(const NotificationPayload& payload)
{
	static constexpr std::string_view names[] = {
		"GameStart",
		"GameOver",
		"GamePaused",
		"TeamDied",
		"TeamChanged",
		"PlayerChanged",
		"PlayerAdded",
		"PlayerRemoved",
		"UnitCreated",
		"UnitFinished",
		"UnitFromFactory",
		"UnitReverseBuilt",
		"UnitConstructionDecayed",
		"UnitDestroyed",
		"UnitTaken",
		"UnitGiven",
		"UnitIdle",
		"UnitCommand",
		"UnitCmdDone",
		"UnitDamaged",
		"UnitStunned",
		"UnitExperience",
		"UnitHarvestStorageFull",
		"UnitSeismicPing",
		"UnitEnteredRadar",
		"UnitEnteredLos",
		"UnitLeftRadar",
		"UnitLeftLos",
		"UnitEnteredUnderwater",
		"UnitEnteredWater",
		"UnitEnteredAir",
		"UnitLeftUnderwater",
		"UnitLeftWater",
		"UnitLeftAir",
		"UnitLoaded",
		"UnitUnloaded",
		"UnitCloaked",
		"UnitDecloaked",
		"UnitMoved",
		"UnitMoveFailed",
		"UnitArrivedAtGoal",
		"FeatureCreated",
		"FeatureDestroyed",
		"FeatureDamaged",
		"FeatureMoved",
		"ProjectileCreated",
		"ProjectileDestroyed",
		"StockpileChanged",
		"RenderUnitDestroyed",
	};
	return payload.valueless_by_exception() ? "invalid" : names[payload.index()];
}

}
