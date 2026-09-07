/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SerialVisualFrame.h"
#include "SystemNext/Modes/Mode.h"
#include "SystemNext/Modes/ModeBinding.h"

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
 * Display state belongs to a binding generation, not a reusable mode address.
 * A callback that replaces a prepared binding stops this frame. A replacement
 * selected before guarded preparation may prepare/render once in this iteration.
 */
ApplicationStatus SerialVisualFrame::ExecuteModeFrame(ModeBinding& modes, ModeFrame& frame)
{
	DrawScope drawScope(*this);
	struct FrameLifetime {
		ModeFrame& frame;
		~FrameLifetime() { frame.data.reset(); }
	} lifetime{frame};
	const auto visual = modes.Select();
	if (frame.bindingGeneration != visual.generation)
		frame.data.reset();
	frame.bindingGeneration = visual.generation;
	auto displayStatus = ApplicationStatus::Continue;
	if (frame.allowRender && visual.mode != nullptr &&
		visual.mode->GetDisplayPhase() == DisplayPhase::WithGraphics) {
		displayStatus = visual.mode->UpdateDisplay(frame);
		if (displayStatus == ApplicationStatus::Blocked)
			throw IncompleteFlow(frame.blocked.id, frame.blocked.reason);
		frame.allowRender &= displayStatus != ApplicationStatus::ExitRequested;
	}

	// [ render ] Never consume data prepared for a retired or reused binding.
	if (!(modes.Select() == visual))
		throw IncompleteFlow("MODE-CHANGED-DURING-DISPLAY", "Display callback replaced its frame binding; no render or present performed");
	const auto rendered = frame.allowRender && visual.mode != nullptr
		? visual.mode->Render(frame) : RenderResult::Skipped();
	if (rendered.state == RenderState::Blocked)
		throw IncompleteFlow(rendered.blocked.id, rendered.blocked.reason);
	if (!(modes.Select() == visual))
		throw IncompleteFlow("MODE-CHANGED-DURING-RENDER", "Render callback replaced its frame binding; no partial output presented");

	// [ present ] Cross-phase scopes end before window presentation.
	frame.data.reset();
	Present(rendered.AllowPresent());
	return displayStatus;
}
}
