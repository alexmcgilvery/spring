/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string_view>

#include "SystemNext/Session/IRuntimeMode.h"

namespace runtime {
class ILoadingServices;
class LoadingVisuals;
enum class LoadingUpdate { Pending, ControllerRetired };

/**
 * Deliver loading progress and complete the transition into a playable session.
 * Ordinary session iterations and synchronous loading callbacks are separate
 * entry points. Neither advances authoritative simulation. Existing controller
 * storage owns the loading process; this mode survives backing retirement.
 * Abstract service resolution deliberately leaves this block-out unregistered.
 */
class LoadingMode : public IRuntimeMode {
public:
	bool HandlesSession() const final;
	SessionUpdate UpdateSession(Session& session) final;
	void ReportProgress(std::string_view text, bool replaceLast, LoadingVisuals& visuals);

protected:
	virtual ILoadingServices& ResolveLoadingServices() = 0;

private:
	LoadingUpdate UpdateLoading();
};

//FIXME [LOAD-004] CLoadScreen::Init/CreateDeleteInstance can finish the entire
// ST load before an ordinary loop iteration. Activation therefore needs an
// explicit startup owner, not a first-UpdateSession initialization shortcut.
// Preserve pregame setup, heartbeat start, game allocation, font/load locking,
// intro creation and synchronous completion ordering. Define partial-init
// cleanup and fake adjacent-mode transitions before binding this mode.
}
