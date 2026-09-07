/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SimulationContext.h"

namespace runtime {
/*
 * Game-local authoritative frame outline. Other modes do not own gameplay simulation;
 * publication consumers do not make the simulation producer application-global.
 */
class Simulation {
public:
	void Step(const SimulationStepContext& context);
};
}
