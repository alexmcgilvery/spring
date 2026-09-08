/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SelectMenuSnapshots.h"
#include "../Mode.h"

namespace runtime {

/**
 * SelectMenu behavior outline with executable typed dispatch.
 * Display is absent by design; Render consumes Session directly.
 * Bodies return no publication until their documented behavior is implemented.
 */
class SelectMenuMode final : public Mode<SelectMenuMode, SelectMenuContracts> {
public:
	SelectMenuMode();

	InputPublication Input(const InputSnapshots& snapshots);
	SessionPublication Session(const SessionSnapshots& snapshots);
	RenderPublication Render(const RenderSnapshots& snapshots);
};

} // namespace runtime
