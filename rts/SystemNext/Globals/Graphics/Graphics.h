/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "GraphicsContext.h"

namespace runtime {
/*
 * Shared graphics resources and window presentation expectations. Mode-specific content
 * stays in each Render block; shared ownership does not imply a separate render thread.
 */
class Graphics {
public:
	void Synchronize(const GraphicsSynchronizationContext& context);
	void Present(const PresentContext& context);
};
}
