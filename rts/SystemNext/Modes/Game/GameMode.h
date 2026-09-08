/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "GameSnapshots.h"
#include "../Mode.h"

namespace runtime {

/**
 * Game behavior outline with executable typed dispatch.
 * Input, Session, Display and Render are independently declared concerns.
 * Bodies return no publication until their documented behavior is implemented.
 */
class GameMode final : public Mode<GameMode, GameContracts> {
public:
	GameMode();

	InputPublication Input(const InputSnapshots& snapshots);
	SessionPublication Session(const SessionSnapshots& snapshots);
	DisplayPublication Display(const DisplaySnapshots& snapshots);
	RenderPublication Render(const RenderSnapshots& snapshots);
};

} // namespace runtime
