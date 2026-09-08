/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "ApplicationContext.h"
#include "Globals/InvocationContext.h"

namespace runtime {

// Forward declarations: requests cross the Session/lifecycle boundary by value.
struct LifecycleRequest;

/**
 * Serial execution of independently identified logical and visual iterations.
 * Stage methods take IDs; the manager owns immutable input association.
 */
class ApplicationLoop {
public:
	explicit ApplicationLoop(ApplicationContext context);

	void Run();

private:
	bool UpdateLogic(LogicalIterationId iteration);
	void UpdateVisuals(VisualIterationId iteration);

	void Input(LogicalIterationId iteration);
	void Session(LogicalIterationId iteration);
	void Display(VisualIterationId iteration);
	void Render(VisualIterationId iteration);
	void Present(VisualIterationId iteration);

	void CommitLifecycle(const LifecycleRequest& request);
	void Shutdown();

private:
	ApplicationContext context;
	bool exitRequested = false;
};

} // namespace runtime
