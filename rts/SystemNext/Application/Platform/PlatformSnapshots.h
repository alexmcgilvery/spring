/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace runtime {

/** Ordered native facts. Interpretation remains with the active mode's Input concern. */
struct PlatformEvent {
public:
	std::uint64_t sequence = 0;
	std::string kind;
	std::string text;
	std::vector<double> values;
};

struct PlatformInputSnapshot {
public:
	std::vector<PlatformEvent> events;
	std::chrono::nanoseconds sampledAt {};
	std::chrono::nanoseconds realDelta {};
};

/** Window observations are independent of mode activation and graphics resources. */
struct WindowSnapshot {
public:
	std::uint64_t windowId = 0;
	std::uint64_t generation = 0;
	unsigned width = 0;
	unsigned height = 0;
	bool focused = false;
	bool visible = false;
};

struct PlatformPublications {
public:
	PlatformInputSnapshot input;
	WindowSnapshot window;
};

} // namespace runtime
