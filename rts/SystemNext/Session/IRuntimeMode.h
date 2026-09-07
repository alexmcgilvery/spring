/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SessionUpdate.h"

namespace runtime {
class Session;
/**
 * Supply session behavior for the current runtime mode.
 * Modes without a session leave their interaction updates to presentation.
 * Simulation follows session authority, so there is no separate simulation
 * enable flag. This contract does not collect input, render or present.
 */
class IRuntimeMode {
public:
	virtual ~IRuntimeMode() = default;
	virtual bool HandlesSession() const = 0;
	virtual SessionUpdate UpdateSession(Session& session) = 0;
};
}
