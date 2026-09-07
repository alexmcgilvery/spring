/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {
class IGameVisualServices;

/**
 * Prepare and draw one game view synchronously under the runtime's draw guard.
 * This companion owns ordering, not world state or window presentation. The
 * supplied services remain abstract and unbound. Render completion reports swap
 * eligibility; ordinary window present remains the surrounding loop's concern.
 */
class GameVisuals {
public:
	explicit GameVisuals(IGameVisualServices& services);
	bool PrepareAndRender();

private:
	bool PrepareClientState();
	void UpdateClientAndGraphicsState(bool newSimulationFrame, bool forceClientUpdate);
	bool RenderFrame();
	void RenderWorld();
	void RenderInterface();
	IGameVisualServices& services;
};
}
