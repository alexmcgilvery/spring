/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../InvocationContext.h"
#include <string>
class SpringApp;
class ConfigHandler;
class CGlobalUnsynced;
namespace runtime {
/** Owned requests, not permission to retire resources from inside a callback. */
struct LifecycleRequests {
	bool exitRequested = false;
	bool reloadRequested = false;
	std::string reloadScript;
};
struct InitializationContext {
	const InvocationContext& invocation;
	SpringApp& host;
	ConfigHandler& configuration;
	LifecycleRequests& requests;
};
struct ReloadContext {
	const InvocationContext& invocation;
	SpringApp& host;
	CGlobalUnsynced& client;
	LifecycleRequests& requests;
};
struct ShutdownContext {
	const InvocationContext& invocation;
	SpringApp& host;
	LifecycleRequests& requests;
	bool initializationCompleted;
};
/* Host references do not expose private SpringApp methods. Construction,
 * resource retirement and routing these requests remain adaptation work. */
}
