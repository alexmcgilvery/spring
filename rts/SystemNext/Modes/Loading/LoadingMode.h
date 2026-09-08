/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "LoadingSnapshots.h"
#include "../Mode.h"

namespace runtime {

/**
 * Loading behavior outline with executable typed dispatch.
 * Input, Session, Display and Render are independently declared concerns.
 * Bodies return no publication until their documented behavior is implemented.
 */
class LoadingMode final : public Mode<LoadingMode, LoadingContracts> {
public:
	LoadingMode();

	InputPublication Input(const InputSnapshots& snapshots);
	SessionPublication Session(const SessionSnapshots& snapshots);
	DisplayPublication Display(const DisplaySnapshots& snapshots);
	RenderPublication Render(const RenderSnapshots& snapshots);
};

} // namespace runtime
