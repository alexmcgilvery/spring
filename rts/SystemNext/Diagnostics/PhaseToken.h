/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "BoundedTraceBuffer.h"

namespace runtime {
/** Owned bookkeeping for one nested trace scope; a zero scope is inactive. */
struct PhaseToken {
	Phase phase = Phase::Host;
	std::uint64_t scope = 0;
	std::uint64_t parent = 0;
	int exceptions = 0;
};
}
