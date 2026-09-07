/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {
class IPreGameServices;

/**
 * Display connection and content-check progress without servicing the session.
 * Loading synchronization and ordinary present belong to the caller. This
 * abstract companion remains unbound until the live-state access is audited.
 */
class PreGameVisuals {
public:
	virtual ~PreGameVisuals() = default;
	bool PrepareAndRender();

protected:
	virtual IPreGameServices& ConnectionServices() = 0;
};
}
