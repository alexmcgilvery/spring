/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../InvocationContext.h"

#include "../Snapshots/SnapshotTypes.h"

namespace runtime {

// Forward declarations: application-owned configuration and resources.
class ApplicationConfiguration;
class ApplicationResources;

/** Private lifecycle bookkeeping; modes issue decisions through SessionOutput. */
struct ApplicationLifecycleState {
public:
	bool initialized = false;
	bool shuttingDown = false;
};

struct InitializationContext {
public:
	const InvocationContext& invocation;
	const ApplicationConfiguration& configuration;
	ApplicationResources& resources;
	ApplicationLifecycleState& state;
};

struct ReloadContext {
public:
	const InvocationContext& invocation;
	const ApplicationConfiguration& configuration;
	ApplicationResources& resources;
	ApplicationLifecycleState& state;
	const Handoff& handoff;
};

struct ShutdownContext {
public:
	const InvocationContext& invocation;
	ApplicationResources& resources;
	ApplicationLifecycleState& state;
	bool initializationCompleted;
};

} // namespace runtime
