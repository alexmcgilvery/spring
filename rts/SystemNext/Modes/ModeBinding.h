/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Globals/InvocationContext.h"

#include <memory>

namespace runtime {

// Forward declarations: owned runtime mode lifetime.
class IMode;

/** A copied binding leases the mode; activation identity, not address, defines continuity. */
struct ActiveModeBinding {
public:
	bool operator==(const ActiveModeBinding&) const = default;

public:
	std::shared_ptr<IMode> mode;
	ModeIdentity identity;
};

} // namespace runtime
