/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "LifecycleContext.h"

namespace runtime {

/*
 * Application-wide lifecycle outline. Connection establishment, loading completion and
 * other mode-local transitions remain documented with their modes.
 */
class Lifecycle {
public:
	void Initialize(const InitializationContext& context);
	void Reload(const ReloadContext& context);
	void Shutdown(const ShutdownContext& context);
};

} // namespace runtime
