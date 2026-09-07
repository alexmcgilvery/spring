/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

//FIXME [SELECT-INPUT-01] Disabled input extraction: the GUI still owns callbacks
// on SelectMenu. Retain its state and actual operations until callback dispatch,
// replacement, and deferred removal can bind safely to this external service.
#if 0
#include <string>
#include "Menu/SelectMenu.h"
namespace runtime {
class LegacySelectMenuInput {
public:
	void SelectReplay();
	void SelectSave();
	void SelectLocalGame();
	void RequestExit();
	void ShowConnection(bool show);
	void ShowSettings(bool show, std::string name);
	void ListSettings();
	void SelectSetting(std::string setting);
	void CloseSettingsList();
	void ConnectToServer(const std::string& addr);
	bool HandleEvent(const SDL_Event& ev);
	void BuildMenuInterface();
	void RetireMenuInterface();
private:
	SelectMenu& ResolveMenu();
};
}
#endif
