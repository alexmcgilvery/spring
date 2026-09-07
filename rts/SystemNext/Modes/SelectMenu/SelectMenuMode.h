/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../IMode.h"

namespace runtime {
/*
 * Built-in menu concern outline. Session and recurring display maintenance are absent by
 * design; the legacy base update is a no-op. Menu-specific construction, actions and
 * retirement are described within input and render, not additional skeleton functions.
 */
class SelectMenuMode final : public IMode {
public:
	SelectMenuMode();
	void Input(const ModeInputContext& context) override;
	void Render(const ModeRenderContext& context) override;
};
}
