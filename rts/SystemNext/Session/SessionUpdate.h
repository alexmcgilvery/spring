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
//FIXME(MODE-INTEGRATION-01): Separate phase completion from session capability.
// SerialVisualFrame::ExecuteFrame uses NoSession to decide whether client-mode
// update still needs to run. Owned modes will describe session and presentation
// work separately; interpreting HandlesSession as "all client work completed"
// would suppress required presentation, while returning NoSession after session
// service could update a replacement twice. Define explicit completion facts
// before wiring the block-outs, with game/loading/menu transition trace tests.
//
//FIXME(MODE-INTEGRATION-02): Define binding lifetime independently of this value.
// The current context has no mode-instance identity and conveys no backing-object
// lease. PreGame can delete itself in packet handling, Loading can retire during
// completion, and menu GUI removal is deferred. Do not use this copied context
// to authorize later reads of an earlier controller. Before activation, specify
// mode retirement and post-guard visual rebinding, including same-address reuse,
// cancellation and loading-progress reentrancy. Existing serial execution remains
// unchanged until those tests and binding contracts exist.
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
