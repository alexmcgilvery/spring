/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Modes/ModeIdentity.h"

#include <chrono>
#include <cstdint>

namespace runtime {

struct LogicalIterationId {
public:
	std::uint64_t value = 0;

	bool operator==(const LogicalIterationId&) const = default;
};

struct VisualIterationId {
public:
	std::uint64_t value = 0;

	bool operator==(const VisualIterationId&) const = default;
};

enum class Flow {
	Logical,
	Visual,
};

struct IterationTiming {
public:
	std::chrono::nanoseconds sampledAt {};
	std::chrono::nanoseconds realDelta {};
};

/** Identity and timing for one admitted concern invocation. */
struct InvocationMetadata {
public:
	ModeIdentity mode;
	Flow flow = Flow::Logical;
	std::uint64_t iteration = 0;
	std::chrono::nanoseconds sampledAt {};
	std::chrono::nanoseconds realDelta {};
};

} // namespace runtime
