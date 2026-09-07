/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Session/IRuntimeMode.h"

namespace runtime {
class IGameServices;

/**
 * Service the established game session; authoritative messages own tick production.
 * The external backing retains game/network state. This unbound mode is abstract
 * until an adapter supplies extracted operations. Visual work has a separate
 * companion; neither this mode nor Session introduces a local tick accumulator.
 */
class GameMode : public IRuntimeMode {
public:
	bool HandlesSession() const final;
	SessionUpdate UpdateSession(Session& session) final;

protected:
	virtual IGameServices& GameServices() = 0;

private:
	void ServiceSession(IGameServices& services);
};
}
