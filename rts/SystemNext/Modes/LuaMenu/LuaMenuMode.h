/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
#include "SystemNext/Modes/Mode.h"
namespace runtime {
/** Lua-backed menu input, client display and graphics; no session work. */
//FIXME ADAPTER-LUAMENU-BACKING: The implementation copy currently
// reads private legacy fields/helpers. Its added friendship and legacy-side
// forwarding have been removed. Supply backing state/access entirely in
// SystemNext before compiling or connecting this adapter; legacy stays intact.
class LuaMenuMode final : public Mode {
public:
	bool HandlesSession() const override;
	DisplayPhase GetDisplayPhase() const override;
	ApplicationStatus UpdateDisplay(ModeFrame&) override;
	RenderResult Render(ModeFrame&) override;
	bool Reset();
	bool Activate(const std::string& msg);
	void ResizeEvent() override;
	int KeyReleased(int keyCode, int scanCode) override;
	int KeyPressed(int keyCode, int scanCode, bool isRepeat) override;
	int TextInput(const std::string& utf8Text) override;
	int TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length) override;
};
}
