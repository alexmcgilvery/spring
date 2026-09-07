/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../IMode.h"

namespace runtime {
/*
 * Loading concern outline covering ordinary iterations and progress-driven work. Startup,
 * worker lifetime and completion are mode-local. Shared graphics supplies
 * context/synchronization and present, but no execution mechanism is implemented here.
 */
class LoadingMode final : public IMode {
public:
	LoadingMode();
	void Input(const ModeInputContext& context) override;
	void Session(const ModeSessionContext& context) override;
	void Display(const ModeDisplayContext& context) override;
	void Render(const ModeRenderContext& context) override;
};
}
