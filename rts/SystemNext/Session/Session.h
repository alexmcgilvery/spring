/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SessionUpdate.h"

namespace runtime {
/**
 * Advance an authoritative session while preserving command/tick stream order.
 * The backing implementation owns timing, accepted commands and permitted ticks;
 * callers must not add a local clock, pre-batch commands, or drive this from draw.
 * Existing inline effects remain part of advancement until explicitly migrated.
 */
class Session {
public:
	virtual ~Session() = default;
	virtual SessionUpdate Advance() = 0;
};
}
