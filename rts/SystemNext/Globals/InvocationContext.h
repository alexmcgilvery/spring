/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <chrono>
#include <cstdint>

namespace runtime {

enum class ModeKind {
	Inactive,
	SelectMenu,
	LuaMenu,
	PreGame,
	Loading,
	Game,
};

/** Every activation gets a new generation, including reuse of the same object. */
struct ModeIdentity {
public:
	bool operator==(const ModeIdentity&) const = default;

public:
	ModeKind kind = ModeKind::Inactive;
	std::uint64_t generation = 0;
};

struct LogicalIterationId {
public:
	bool operator==(const LogicalIterationId&) const = default;

public:
	std::uint64_t value = 0;
};

struct VisualIterationId {
public:
	bool operator==(const VisualIterationId&) const = default;

public:
	std::uint64_t value = 0;
};

enum class Flow {
	Logical,
	Visual,
};

/** One clock sample per flow invocation; downstream stages reuse these facts. */
struct IterationTiming {
public:
	std::chrono::nanoseconds sampledAt {};
	std::chrono::nanoseconds realDelta {};
};

/** Immutable invocation facts. These values grant no mutable resource access. */
struct InvocationContext {
public:
	ModeIdentity mode;
	Flow flow = Flow::Logical;
	std::uint64_t iteration = 0;
	std::chrono::nanoseconds sampledAt {};
	std::chrono::nanoseconds realDelta {};
};

} // namespace runtime
