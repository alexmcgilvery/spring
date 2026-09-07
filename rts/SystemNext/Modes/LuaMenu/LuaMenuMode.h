/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Session/IRuntimeMode.h"

namespace runtime {
class ILuaMenuServices;
/**
 * Maintain script-defined menu interaction without authoritative session work.
 *
 * Client update runs before the outer loading/draw guard. Existing input routes
 * remain separate: keyboard goes through LuaInputReceiver, text through event
 * dispatch, and resize through ViewResize. The mode does not poll input again.
 * This research type remains abstract and unregistered; its operation ordering
 * can be exercised with substitute services without activating a real Lua menu.
 */
class LuaMenuMode : public IRuntimeMode {
public:
	explicit LuaMenuMode(ILuaMenuServices& services);
	bool HandlesSession() const final;
	SessionUpdate UpdateSession(Session& session) override = 0;
	ApplicationStatus UpdateClientState();

private:
	void MaintainClientServices();
	void UpdateMenuInteraction();
	ILuaMenuServices& services;
};
}
