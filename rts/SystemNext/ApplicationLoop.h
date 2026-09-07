/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
#include "ApplicationContext.h"
#include <cstdint>
namespace runtime {
/**
 * Connected serial orchestration over explicit host, binding and context contracts.
 * Modes still contain concern outlines; this code is not registered with the engine.
 * Platform adapters, eligibility/results and loading-progress execution remain work.
 */
class ApplicationLoop {
public:
	void Run(const ApplicationContext& context);
private:
	void Update(const ApplicationContext& context, std::uint64_t iteration);
};
}
