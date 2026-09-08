/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Snapshots/SnapshotReads.h"
#include "Simulation/Publication/PublishedFrame.h"

namespace runtime {

// Payload outlines: schemas and extraction remain unimplemented; no empty frame is published.
struct GameInputSnapshot {
public:
	// Expected input information is documented in GameMode::Input().
};

struct GameSessionSnapshot {
public:
	// Expected session information is documented in GameMode::Session().
};

struct GameDisplaySnapshot {
public:
	// Expected display information is documented in GameMode::Display().
};

/** Compile-time input contracts; these declarations do not implement mode behavior. */
struct GameContracts : ModeContracts {
public:
	using InputData = GameInputSnapshot;
	using SessionData = GameSessionSnapshot;
	using DisplayData = GameDisplaySnapshot;
	using SimulationData = PublishedSimFrame;

	using InputReads = SnapshotReads<
		Required<Stage::PlatformInput, Slot::Current>,
		Required<Stage::Window, Slot::Current>,
		Required<Stage::Activation, Slot::Current>,
		Optional<Stage::Session, Slot::Previous>
	>;

	using SessionReads = SnapshotReads<
		Required<Stage::Input, Slot::Current>,
		Required<Stage::Activation, Slot::Current>,
		Optional<Stage::Display, Slot::Previous>
	>;

	using DisplayReads = SnapshotReads<
		Required<Stage::Window, Slot::Current>,
		Required<Stage::GraphicsOutput, Slot::Current>,
		Required<Stage::Session, Slot::Current>,
		Optional<Stage::Display, Slot::Previous>,
		Optional<Stage::Simulation, Slot::Current>,
		Optional<Stage::Simulation, Slot::Previous>
	>;

	using RenderReads = SnapshotReads<
		Required<Stage::GraphicsOutput, Slot::Current>,
		Required<Stage::Display, Slot::Current>
	>;
};

} // namespace runtime
