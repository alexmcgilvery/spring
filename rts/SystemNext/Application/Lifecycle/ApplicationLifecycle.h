/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "LifecycleRequest.h"

#include <memory>

namespace runtime {

// Forward declarations: lifecycle constructs modes without exposing itself to them.
class IMode;

/** Owns initialization, mode construction, reload and shutdown boundaries. */
class ApplicationLifecycle {
public:
	virtual ~ApplicationLifecycle();

	virtual void Initialize() = 0;
	virtual std::shared_ptr<IMode> CreateInitialMode() = 0;
	virtual std::shared_ptr<IMode> CreateMode(const LifecycleRequest& request) = 0;
	virtual void Shutdown() = 0;
};

} // namespace runtime
