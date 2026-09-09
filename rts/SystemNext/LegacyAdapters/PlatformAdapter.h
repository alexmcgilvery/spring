/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Application/Platform/Platform.h"

namespace runtime {

/**
 * Concrete Platform adapter wrapping the existing SDL event collection,
 * window management and lifecycle-flag infrastructure.
 *
 * BeginIteration pumps SDL events, calls the active controller's Update()
 * and Draw(), and swaps buffers. This bridges the existing monolithic frame
 * loop into the new ApplicationLoop architecture while mode concerns remain
 * documented outlines. CollectPublications packages the current window
 * state. ExitRequested/ReloadRequested bridge the existing gu->globalQuit /
 * gu->globalReload flags into the runtime lifecycle.
 */
class PlatformAdapter final : public Platform {
public:
	PlatformAdapter();
	~PlatformAdapter() override;

	bool BeginIteration() override;
	PlatformPublications CollectPublications() override;
	bool ExitRequested() const override;
	bool ReloadRequested() const override;
};

} // namespace runtime
