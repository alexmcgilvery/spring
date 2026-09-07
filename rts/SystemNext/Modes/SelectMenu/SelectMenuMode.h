/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Session/IRuntimeMode.h"

namespace runtime {
/**
 * Select a local game, connection, replay or save without advancing a session.
 *
 * Input callbacks currently mutate the existing SelectionWidget, ClientSetup
 * and settings windows. Preserve those owners; a mode is not another GUI tree
 * or a copy of selected setup state. Enter/leave are distinct from allocation
 * and retirement because GUI callbacks can replace the active mode before the
 * menu and its children are deleted by deferred GUI cleanup.
 *
 * Activation builds the GUI, selects a background using the unsynced RNG and
 * mounts menu content. Input dispatch invokes selection/settings/start/quit
 * callbacks. There is no per-iteration client update: SelectMenu inherits the
 * controller's no-op update. Visual work belongs to SelectMenuVisuals.
 *
 * This researched block-out remains abstract and is not a runtime binding.
 */
class SelectMenuMode : public IRuntimeMode {
public:
	bool HandlesSession() const final;

	//FIXME [SELECT-MENU-01] The shared IRuntimeMode contract requires a session
	// method even for a mode which must never service a session. Keep it abstract
	// until activation is wired; resolve this by a capability-specific interface
	// or an explicitly tested unreachable implementation, not invented menu work.
	// Verify runtime capability checks never call this entry for SelectMenu.
	SessionUpdate UpdateSession(Session& session) override = 0;
};
}
