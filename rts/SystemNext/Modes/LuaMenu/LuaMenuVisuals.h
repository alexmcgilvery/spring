/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {
class ILuaMenuServices;
/**
 * Produce menu graphics after client maintenance and outer draw synchronization.
 *
 * This unregistered visual companion has no present operation and takes no load
 * lock: its caller retains that scope through the ordinary window swap. A true
 * result permits swap; false means the existing skipped-draw sleep completed.
 * Returning false does not ask the application to exit or bypass forced swaps.
 */
class LuaMenuVisuals {
public:
	explicit LuaMenuVisuals(ILuaMenuServices& services);
	bool PrepareAndRender();

private:
	bool ShouldRender();
	void PrepareFrame();
	void RenderMenu();
	ILuaMenuServices& services;
};
}
