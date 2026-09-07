/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
#include "SystemNext/Modes/Mode.h"
union SDL_Event;
namespace runtime {
/** GUI-backed input and rendering; the GUI remains the single state owner. */
//FIXME ADAPTER-SELECTMENU-BACKING: The implementation copy currently
// reads private legacy fields/helpers. Its added friendship and legacy-side
// forwarding have been removed. Supply backing state/access entirely in
// SystemNext before compiling or connecting this adapter; legacy stays intact.
class SelectMenuMode final : public Mode {
public:
	bool HandlesSession() const override;
	DisplayPhase GetDisplayPhase() const override;
	RenderResult Render(ModeFrame&) override;
	void Initialize();
	void Retire();
	void Demo();
	void Load();
	void Single();
	void Quit();
	void ShowConnectWindow(bool show);
	void ShowSettingsWindow(bool show, std::string name);
	void ShowSettingsList();
	void SelectSetting(std::string setting);
	void CleanWindow();
	void DirectConnect(const std::string& addr);
	bool HandleEventSelf(const SDL_Event& ev);
};
}
