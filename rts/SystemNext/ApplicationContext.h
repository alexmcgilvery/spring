/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
namespace runtime {
class ApplicationHost;
class ModeContextProvider;
struct ActiveModeBinding;
/**
 * Dependencies of orchestration. The host owns platform/resources; the provider
 * assembles mode-local contexts; the binding identifies the current activation.
 * None of this bundle is passed wholesale into modes, and no globals are resolved
 * by the loop. Concrete engine integration remains to be implemented.
 */
struct ApplicationContext {
	ApplicationHost& host;
	ModeContextProvider& contexts;
	ActiveModeBinding& activeMode;
};
}
