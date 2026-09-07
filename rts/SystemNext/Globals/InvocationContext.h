/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <chrono>
#include <cstdint>

namespace runtime {
enum class ModeKind { Inactive, SelectMenu, LuaMenu, PreGame, Loading, Game };

/** Identifies activation, not the address of a controller. Zero is unbound. */
struct ModeIdentity {
	ModeKind kind = ModeKind::Inactive;
	std::uint64_t generation = 0;
};

/**
 * Value metadata sampled by the application boundary for one concern invocation.
 * sampledAt/realDelta use a monotonic clock and describe this concern's sample;
 * they do not replace additional legacy timing samples or freeze referenced state.
 *
 * A context is a synchronous borrow. Referents must exist throughout its call.
 * If a callback retires/replaces a mode, stop using that invocation's borrows and
 * obtain a fresh context. No context may be queued to another thread or retained
 * by a mode. Generations describe this rule; validation/lifetime management are
 * not implemented by these structs. Const access is shallow for legacy objects.
 */
struct InvocationContext {
	ModeIdentity mode;
	std::uint64_t iteration;
	std::chrono::nanoseconds sampledAt;
	std::chrono::nanoseconds realDelta;
};
}
