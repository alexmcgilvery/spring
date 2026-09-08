/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "PreGameSnapshots.h"
#include "../Mode.h"

namespace runtime {

/**
 * PreGame behavior outline with executable typed dispatch.
 * Display is absent by design; Render consumes Session directly.
 * Bodies return no publication until their documented behavior is implemented.
 */
class PreGameMode final : public Mode<PreGameMode, PreGameContracts> {
public:
	PreGameMode();

	InputPublication Input(const InputSnapshots& snapshots);
	SessionPublication Session(const SessionSnapshots& snapshots);
	RenderPublication Render(const RenderSnapshots& snapshots);
};

} // namespace runtime
