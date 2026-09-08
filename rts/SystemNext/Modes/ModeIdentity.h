/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

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

/** A new generation is assigned for every activation, including object reuse. */
struct ModeIdentity {
public:
	ModeKind kind = ModeKind::Inactive;
	std::uint64_t generation = 0;

	bool operator==(const ModeIdentity&) const = default;
};

} // namespace runtime
