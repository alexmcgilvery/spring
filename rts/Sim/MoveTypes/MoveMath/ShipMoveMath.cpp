/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveMath.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

#include "System/Misc/TracyDefs.h"

/*
Calculate speed-multiplier for given height and slope data.
*/
float CMoveMath::ShipSpeedMod(const MoveDef& moveDef, float height, float slope)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (-height < moveDef.depth)
		return 0.0f;

	return 1.0f;
}

float CMoveMath::ShipSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// uphill slopes can lead even closer to shore, so
	// block movement if we are above our minWaterDepth
	if (height >= 0.0f || ((dirSlopeMod >= 0.0f) && (-height < moveDef.depth)))
		return 0.0f;

	return 1.0f;
}

