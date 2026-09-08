/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "ObservationContext.h"

namespace runtime {

/*
 * Game-local simulation notification outline. This describes semantic observation; application
 * diagnostics later records cross-mode runtime measurements and failures.
 */
class Observation {
public:
	void Observe(const SimulationObservationContext& context);
};

} // namespace runtime
