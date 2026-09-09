/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Application/Graphics/Graphics.h"

namespace runtime {

/**
 * Concrete Graphics adapter wrapping the existing globalRendering backend.
 *
 * In this initial integration, the actual controller Draw and SwapBuffers
 * calls are handled by PlatformAdapter::BeginIteration(). PlanVisuals
 * returns None so the ApplicationLoop's visual chain is a no-op. This will
 * change once mode concerns are implemented to describe render work.
 */
class GraphicsAdapter final : public Graphics {
public:
	GraphicsAdapter();
	~GraphicsAdapter() override;

	VisualPlan PlanVisuals(const ModeIdentity& mode) override;
	std::optional<RenderedOutput> Render(
		const GraphicsOutputSnapshot& target,
		std::unique_ptr<const RenderCommands> commands
	) override;
	PresentationReceipt Present(const RenderedOutput& output) override;
};

} // namespace runtime
