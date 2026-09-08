/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Snapshots/SnapshotReads.h"

namespace runtime {

// Payload outlines: schemas and extraction remain unimplemented; no empty frame is published.
struct LoadingInputSnapshot {
public:
	// Expected input information is documented in LoadingMode::Input().
};

struct LoadingSessionSnapshot {
public:
	// Expected session information is documented in LoadingMode::Session().
};

struct LoadingDisplaySnapshot {
public:
	// Expected display information is documented in LoadingMode::Display().
};

/** Compile-time input contracts; these declarations do not implement mode behavior. */
struct LoadingContracts : ModeContracts {
public:
	using InputData = LoadingInputSnapshot;
	using SessionData = LoadingSessionSnapshot;
	using DisplayData = LoadingDisplaySnapshot;

	using InputReads = SnapshotReads<
		Required<Stage::PlatformInput, Slot::Current>,
		Required<Stage::Window, Slot::Current>,
		Required<Stage::Activation, Slot::Current>,
		Optional<Stage::Session, Slot::Previous>
	>;

	using SessionReads = SnapshotReads<
		Required<Stage::Input, Slot::Current>,
		Required<Stage::Activation, Slot::Current>
	>;

	using DisplayReads = SnapshotReads<
		Required<Stage::Window, Slot::Current>,
		Required<Stage::GraphicsOutput, Slot::Current>,
		Required<Stage::Session, Slot::Current>,
		Optional<Stage::Display, Slot::Previous>
	>;

	using RenderReads = SnapshotReads<
		Required<Stage::GraphicsOutput, Slot::Current>,
		Required<Stage::Display, Slot::Current>
	>;
};

} // namespace runtime
