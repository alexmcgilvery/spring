/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Session/SessionUpdate.h"

namespace runtime {
class ModeBinding;
struct ModeFrame;
/**
 * Execute presentation, rendering and display presentation as one visual unit.
 * The return value reports client-mode exit requests, never rendering success.
 * Current execution is synchronous and may read live state. Independent
 * scheduling requires owning visual inputs and removal of conflicting live
 * access; VisualFrameContext alone does not establish that boundary.
 */
class IVisualFrame {
public:
	virtual ~IVisualFrame() = default;
	virtual ApplicationStatus ExecuteModeFrame(ModeBinding& modes, ModeFrame& frame) = 0;
};
}
