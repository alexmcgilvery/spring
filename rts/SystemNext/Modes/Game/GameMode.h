/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../IMode.h"

namespace runtime {
/*
 * Gameplay concern outline. Simulation execution, publication and observation are owned
 * beneath this mode. Input/session/display/render are conceptual blocks; their relationship
 * to current callback timing is documentation, not an implemented separation.
 */
class GameMode final : public IMode {
public:
	GameMode();
	void Input(const ModeInputContext& context) override;
	void Session(const ModeSessionContext& context) override;
	void Display(const ModeDisplayContext& context) override;
	void Render(const ModeRenderContext& context) override;
};
}
