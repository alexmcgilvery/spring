/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <memory>
#include "SystemNext/Session/SessionUpdate.h"

namespace runtime {
enum class DisplayPhase { None, BeforeGraphics, WithGraphics };

/** Per-invocation mode data, destroyed before present and before guard release. */
class ModeFrameData {
public:
	virtual ~ModeFrameData();
};

/** Decisions are not leases on mutable controller state or render snapshots. */
struct ModeFrame {
	bool allowRender = true;
	std::uint64_t bindingGeneration = 0;
	BlockedFlow blocked;
	std::unique_ptr<ModeFrameData> data;
	ApplicationStatus Block(std::string_view id, std::string_view reason);
};
}
