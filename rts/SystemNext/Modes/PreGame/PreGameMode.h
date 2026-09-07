/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Session/IRuntimeMode.h"

namespace runtime {
class IPreGameServices;

/**
 * Establish a session without advancing simulation or rendering its output.
 *
 * Source-derived block-out for the CPreGame responsibility. Operations remain
 * unbound: this abstract mode is not registered with the running application.
 * The backing connection owns host/join/demo/save state and pending setup work.
 */
class PreGameMode : public IRuntimeMode {
public:
	bool HandlesSession() const final;
	SessionUpdate UpdateSession(Session& session) final;

protected:
	/** Return an external service whose lifetime survives backing retirement. */
	virtual IPreGameServices& ConnectionServices() = 0;

private:
	void ServiceConnection(IPreGameServices& services);
};
}
