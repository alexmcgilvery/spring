/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Globals/InvocationContext.h"
#include "../Globals/Graphics/VisualOutput.h"
#include "../Globals/Snapshots/SnapshotManager.h"

#include <memory>

namespace runtime {

// Render product: invocation-owned admission spans command preparation and execution.
struct RenderWork {
public:
	InvocationLease invocation;
	std::unique_ptr<const RenderCommands> commands;
};

/**
 * Runtime mode interface. The generated Mode bridge implements dispatch once;
 * concrete modes define only their typed concern functions. No capability flags
 * duplicate the presence or absence of those functions.
 */
class IMode {
public:
	virtual ~IMode();

	virtual void RegisterSnapshots(SnapshotManager& snapshots) const = 0;
	virtual void ExecuteInput(SnapshotManager& snapshots, LogicalIterationId iteration) = 0;
	virtual void ExecuteSession(SnapshotManager& snapshots, LogicalIterationId iteration) = 0;
	virtual void ExecuteDisplay(SnapshotManager& snapshots, VisualIterationId iteration) = 0;
	virtual std::optional<RenderWork> DescribeRender(SnapshotManager& snapshots, VisualIterationId iteration) = 0;

protected:
	explicit IMode(ModeKind kind);

public:
	const ModeKind kind;
};

} // namespace runtime
