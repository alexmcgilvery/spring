/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../../Globals/InvocationContext.h"
class CGame;
class CGlobalSynced;
class CUnitHandler;
class CFeatureHandler;
class CProjectileHandler;
class CEventHandler;
namespace runtime {
struct SimulationStepContext {
	const InvocationContext& invocation;
	CGame& game;
	CGlobalSynced& simulation;
	CUnitHandler& units;
	CFeatureHandler& features;
	CProjectileHandler& projectiles;
	CEventHandler& events;
	int acceptedTick;
};
/* Only the authoritative session caller supplies acceptedTick. This is not a
 * clock or permission for the outer loop to invent a tick. Mutable world access
 * belongs inside existing synced scopes; RNG/checksum and tick-bound client
 * effects still require explicit adaptation before this outline can execute. */
}
