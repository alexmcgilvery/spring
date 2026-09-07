/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Modes/Mode.h"

namespace runtime {
/**
 * Own session, display, rendering and input order while CGame retains world state.
 * Authoritative messages alone produce ticks. Display includes synchronous Lua,
 * audio and graphics updates and therefore runs inside the graphics guard. Input
 * submission keeps its existing late position until command timing is changed
 * deliberately. Per-invocation frame storage owns samples and the draw scope.
 */
//FIXME ADAPTER-GAME-BACKING: The implementation copy currently
// reads private legacy fields/helpers. Its added friendship and legacy-side
// forwarding have been removed. Supply backing state/access entirely in
// SystemNext before compiling or connecting this adapter; legacy stays intact.
class GameMode final : public Mode {
public:
	bool HandlesSession() const final;
	DisplayPhase GetDisplayPhase() const final;
	SessionUpdate UpdateSession(Session& session) override;
	ApplicationStatus UpdateDisplay(ModeFrame& frame) override;
	RenderResult Render(ModeFrame& frame) override;
	void ResizeEvent() override;
	int KeyPressed(int keyCode, int scanCode, bool isRepeat) override;
	int KeyReleased(int keyCode, int scanCode) override;
	CInputReceiver* GetInputReceiver() override;
	int KeyMapChanged() override;
	int TextInput(const std::string& utf8Text) override;
	int TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length) override;

private:
	void SubmitPendingInput();
	void RenderWorld();
	void RenderInterface();
};
}
