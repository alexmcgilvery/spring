/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GraphicsAdapter.h"

#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"

namespace runtime {

GraphicsAdapter::GraphicsAdapter()
{
	LOG("[runtime::GraphicsAdapter] constructed");
}

GraphicsAdapter::~GraphicsAdapter()
{
	LOG("[runtime::GraphicsAdapter] destroyed");
}

VisualPlan GraphicsAdapter::PlanVisuals(const ModeIdentity& mode)
{
	// In this initial integration, the actual controller Draw and SwapBuffers
	// calls are handled by PlatformAdapter::BeginIteration(). Return None so
	// the ApplicationLoop's visual chain is a no-op. This will change once
	// mode concerns are implemented to describe render work.
	return {VisualSchedule::Skip, {}, {}};
}

std::optional<RenderedOutput> GraphicsAdapter::Render(
	const GraphicsOutputSnapshot& target,
	std::unique_ptr<const RenderCommands> commands
)
{
	return std::nullopt;
}

PresentationReceipt GraphicsAdapter::Present(const RenderedOutput& output)
{
	PresentationReceipt receipt;
	receipt.outputId = output.outputId;
	receipt.targetId = output.targetId;
	receipt.targetGeneration = output.targetGeneration;
	receipt.outcome = PresentationOutcome::Skipped;
	return receipt;
}

} // namespace runtime
