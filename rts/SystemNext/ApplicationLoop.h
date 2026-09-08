/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Modes/ModeBinding.h"
#include "Snapshots/InvocationMetadata.h"

#include <memory>
#include <optional>

namespace runtime {

// Forward declarations: application-owned executors are never exposed to modes.
class ApplicationLifecycle;
class Diagnostics;
class Graphics;
class IMode;
class Platform;
class SnapshotManager;

// Forward declarations: values cross named application boundaries by ownership.
struct LifecycleRequest;
struct PlatformPublications;

/** Serial scheduling of independently identified logical and visual iterations. */
class ApplicationLoop {
public:
	ApplicationLoop(
		Platform& platform,
		ApplicationLifecycle& lifecycle,
		SnapshotManager& snapshots,
		Diagnostics& diagnostics,
		Graphics* graphics
	);

	void Run();

private:
	bool UpdateLogic(PlatformPublications publications);
	void UpdateVisuals();

	void Input(LogicalIterationId iteration);
	void Session(LogicalIterationId iteration);
	void Display(VisualIterationId iteration);
	void Render(VisualIterationId iteration);
	void Present(VisualIterationId iteration);

	std::optional<LifecycleRequest> PlatformLifecycleRequest() const;
	void CommitLifecycle(const LifecycleRequest& request);
	void Activate(std::shared_ptr<IMode> mode, const LifecycleRequest* request = nullptr);
	void Shutdown();

private:
	Platform& platform;
	ApplicationLifecycle& lifecycle;
	SnapshotManager& snapshots;
	Diagnostics& diagnostics;
	Graphics* graphics = nullptr;
	ActiveModeBinding activeMode;
	bool exitRequested = false;
};

} // namespace runtime
