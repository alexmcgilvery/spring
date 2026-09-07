/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../IMode.h"

namespace runtime {
/*
 * Lua menu concern outline. Session is absent by design. Input, recurring client display
 * maintenance and rendering are distinct expected blocks; archive/menu activation belongs
 * to this mode.
 */
class LuaMenuMode final : public IMode {
public:
	LuaMenuMode();
	void Input(const ModeInputContext& context) override;
	void Display(const ModeDisplayContext& context) override;
	void Render(const ModeRenderContext& context) override;
};
}
