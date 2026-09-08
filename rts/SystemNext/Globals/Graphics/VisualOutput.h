/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../InvocationContext.h"

#include <memory>
#include <optional>

namespace runtime {

/** Owning, mode-specific descriptions. Concrete command schemas follow adaptation. */
class RenderCommands {
public:
	virtual ~RenderCommands() = 0;
};

/** Backend-owned lifetime, including any required completion synchronization. */
class RenderResource {
public:
	virtual ~RenderResource() = 0;
};

struct RenderedOutput {
public:
	std::uint64_t outputId = 0;
	std::uint64_t targetId = 0;
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
	PresentationOutcome outcome = PresentationOutcome::Skipped;
	std::chrono::nanoseconds startedAt {};
	std::chrono::nanoseconds completedAt {};
};

/** Cumulative schedules. An absent Display does not require a dummy publication. */
enum class VisualSchedule {
	None,
	Display,
	Offscreen,
	Present,
};

/**
 * Application-owned visual execution. Headless applications have no instance.
 *
 * Why: modes describe output independently of devices and windows. PlanIteration
 * chooses stages; Render executes owned commands and returns a leased output;
 * Present consumes that exact output and reports a receipt. None advances logic.
 * Backend handles, synchronization and resource destruction stay private. Output
 * leases must keep resources alive, including after mode and manager retirement.
 *
 * Expected sources: [SpringApp.cpp](../../../System/SpringApp.cpp),
 * SpringApp::Update(); [LoadScreen.cpp](../../../Game/LoadScreen.cpp),
 * CLoadScreen::SetLoadMessage(). These are investigation references. Loading
 * publishes progress; the application schedules visual work instead of recursively
 * rendering or presenting from a progress callback.
 */
class VisualOutput {
public:
	virtual ~VisualOutput();

	virtual VisualSchedule PlanIteration(const InvocationContext& invocation) = 0;
	virtual std::optional<RenderedOutput> Render(std::unique_ptr<const RenderCommands> commands) = 0;
	virtual PresentationReceipt Present(const RenderedOutput& output) = 0;
};

} // namespace runtime
