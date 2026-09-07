/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SerialVisualFrame.h"

namespace runtime {
SerialVisualFrame::DrawScope::DrawScope(SerialVisualFrame& frame): frame(frame)
{
	frame.LockDraw();
}

SerialVisualFrame::DrawScope::~DrawScope()
{
	frame.UnlockDraw();
}

/**
 * Complete visual work even when session or client update requests exit.
 * Client-mode update runs before loading synchronization, matching session
 * update. Drawing and present share the guard because both access graphics
 * state. A false draw decision still reaches present, which may force a swap.
 */
ApplicationStatus SerialVisualFrame::ExecuteFrame(const VisualFrameContext& context)
{
	const auto clientStatus = context.sessionOutcome == SessionOutcome::NoSession
		? UpdateClientMode()
		: ApplicationStatus::Continue;

	DrawScope drawScope(*this);
	const bool allowSwap = PrepareAndRender(context, clientStatus);
	Present(allowSwap);
	return clientStatus;
}

/**
 * Visual eligibility belongs here rather than in session scheduling.
 * Update exit requests suppress draw, but do not suppress the guarded present
 * path. Draw implementations resolve the current mode after update and locking.
 */
bool SerialVisualFrame::PrepareAndRender(const VisualFrameContext& context, ApplicationStatus clientStatus)
{
	if (context.sessionOutcome == SessionOutcome::ExitRequested || clientStatus == ApplicationStatus::ExitRequested)
		return false;
	return Draw();
}
}
