/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {

enum class ApplicationStatus { Continue, ExitRequested };
enum class SessionOutcome { NoSession, Continue, ExitRequested };

/**
 * Facts from session service, not instructions to the renderer.
 * NoSession means client-mode update still needs to run. The other outcomes
 * mean this iteration's mode update has already run, even if it replaced the
 * mode. No live references or immutable render inputs are carried here.
 */
struct VisualFrameContext {
	SessionOutcome sessionOutcome = SessionOutcome::NoSession;
};

/**
 * Separate application continuation from the facts consumed by visual work.
 * A controller exit request must not bypass the visual completion path.
 */
struct SessionUpdate {
	ApplicationStatus applicationStatus;
	VisualFrameContext visualContext;
	static SessionUpdate NoSession();
	static SessionUpdate FromContinuation(bool continueRunning);
};
}
