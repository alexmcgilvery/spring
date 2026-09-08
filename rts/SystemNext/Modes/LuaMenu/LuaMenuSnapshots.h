/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Globals/Snapshots/SnapshotReads.h"

namespace runtime {

// Payload outlines: schemas and extraction remain unimplemented; no empty frame is published.
struct LuaMenuInputSnapshot {
public:
	// Expected input information is documented in LuaMenuMode::Input().
};

struct LuaMenuSessionSnapshot {
public:
	// Expected session information is documented in LuaMenuMode::Session().
};

struct LuaMenuDisplaySnapshot {
public:
	// Expected display information is documented in LuaMenuMode::Display().
};

/** Compile-time input contracts; these declarations do not implement mode behavior. */
struct LuaMenuContracts : ModeContracts {
public:
	using InputData = LuaMenuInputSnapshot;
	using SessionData = LuaMenuSessionSnapshot;
	using DisplayData = LuaMenuDisplaySnapshot;

	using InputReads = SnapshotReads<
		Required<Stage::Application, Slot::Current>,
		Required<Stage::Activation, Slot::Current>,
		Optional<Stage::Session, Slot::Previous>
	>;

	using SessionReads = SnapshotReads<
		Required<Stage::Input, Slot::Current>,
		Required<Stage::Activation, Slot::Current>,
		Optional<Stage::Display, Slot::Previous>
	>;

	using DisplayReads = SnapshotReads<
		Required<Stage::Application, Slot::Current>,
		Required<Stage::Session, Slot::Current>,
		Optional<Stage::Display, Slot::Previous>
	>;

	using RenderReads = SnapshotReads<
		Required<Stage::Application, Slot::Current>,
		Required<Stage::Display, Slot::Current>
	>;
};

} // namespace runtime
