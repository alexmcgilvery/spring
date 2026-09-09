/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Snapshots/InvocationMetadata.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

namespace runtime {

class RenderCommands {
public:
	virtual ~RenderCommands() = 0;
};

class RenderResource {
public:
	virtual ~RenderResource() = 0;
};

struct GraphicsOutputSnapshot {
public:
	std::uint64_t targetId = 0;
	std::uint64_t generation = 0;
	unsigned width = 0;
	unsigned height = 0;
	bool available = false;
};

struct RenderedOutput {
public:
	std::uint64_t outputId = 0;
	std::uint64_t targetId = 0;
	std::uint64_t targetGeneration = 0;
	std::shared_ptr<const RenderResource> resource;
};

enum class PresentationOutcome {
	Presented,
	Skipped,
	Failed,
};

struct PresentationReceipt {
public:
	std::uint64_t outputId = 0;
	std::uint64_t targetId = 0;
	std::uint64_t targetGeneration = 0;
	PresentationOutcome outcome = PresentationOutcome::Skipped;
	std::chrono::nanoseconds startedAt {};
	std::chrono::nanoseconds completedAt {};
};

enum class VisualSchedule {
	Skip,
	Display,
	Offscreen,
	Present,
};

struct VisualPlan {
public:
	VisualSchedule schedule = VisualSchedule::Skip;
	IterationTiming timing;
	GraphicsOutputSnapshot output;
};

/**
 * Owns visual planning, backend execution, output lifetime and presentation.
 *
 * Render and Present are distinct stages over the same target and resource
 * lifetime. Modes receive immutable output facts and only describe content.
 * A headless application has no Graphics instance.
 */
class Graphics {
public:
	virtual ~Graphics();

	virtual VisualPlan PlanVisuals(const ModeIdentity& mode) = 0;
	virtual std::optional<RenderedOutput> Render(
		const GraphicsOutputSnapshot& target,
		std::unique_ptr<const RenderCommands> commands
	) = 0;
	virtual PresentationReceipt Present(const RenderedOutput& output) = 0;
};

} // namespace runtime
