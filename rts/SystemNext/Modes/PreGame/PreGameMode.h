/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../IMode.h"

namespace runtime {
/*
 * Connection/setup concern outline. Independent recurring display work is absent initially.
 * Startup workers, connection progress and transition into Loading belong to session;
 * cancellation belongs to input.
 */
class PreGameMode final : public IMode {
public:
	PreGameMode();
	void Input(const ModeInputContext& context) override;
	void Session(const ModeSessionContext& context) override;
	void Render(const ModeRenderContext& context) override;
};
}
