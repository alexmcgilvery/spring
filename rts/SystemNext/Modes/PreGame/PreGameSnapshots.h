/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Globals/Snapshots/SnapshotReads.h"

namespace runtime {

// Payload outlines: schemas and extraction remain unimplemented; no empty frame is published.
struct PreGameInputSnapshot {
public:
	// Expected input information is documented in PreGameMode::Input().
};

struct PreGameSessionSnapshot {
public:
	// Expected session information is documented in PreGameMode::Session().
};

/** Compile-time input contracts; these declarations do not implement mode behavior. */
struct PreGameContracts : ModeContracts {
public:
	using InputData = PreGameInputSnapshot;
	using SessionData = PreGameSessionSnapshot;

	using InputReads = SnapshotReads<
		Required<Stage::Application, Slot::Current>,
		Required<Stage::Activation, Slot::Current>,
		Optional<Stage::Session, Slot::Previous>
	>;

	using SessionReads = SnapshotReads<
		Required<Stage::Input, Slot::Current>,
		Required<Stage::Activation, Slot::Current>
	>;

	using RenderReads = SnapshotReads<
		Required<Stage::Application, Slot::Current>,
		Required<Stage::Session, Slot::Current>
	>;
};

} // namespace runtime
