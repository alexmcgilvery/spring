/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacySelectMenuInput.h"

//FIXME [SELECT-INPUT-01] This complete disabled draft retains existing owner
// accesses; ResolveMenu/private access and file-local window helper visibility
// are unresolved. Do not enable it or dispatch both old and new input handlers.
// Actual SelectMenu.cpp callback bodies follow; no Update/Draw forwarding occurs.
#if 0
namespace runtime {
// [ input ] Selection actions and keyboard handling keep their existing order.
void LegacySelectMenuInput::SelectReplay()
{
	auto& backing = ResolveMenu();
	//FIXME [SELECT-INPUT-02] This original reference-capturing callback now
	// crosses an extracted input operation. Reference capture does not retain
	// the GUI owner; resolve its lifetime at deferred callback invocation.
	// The exact deferred callback and mutations are retained below for review.
	const auto demoSelectedCB = [&](const std::string& userDemo) {
		if (pregame != nullptr)
			return;

		backing.clientSetup->isHost = true;
		backing.clientSetup->myPlayerName += " (spec)";
		backing.clientSetup->demoFile = userDemo;

		pregame = new CPreGame(backing.clientSetup);
		pregame->AsyncExecute(&CPreGame::LoadDemoFile, backing.clientSetup->demoFile);
		//pregame->LoadDemoFile(backing.clientSetup->demoFile);

		//FIXME [SELECT-INPUT-04] Mode replacement has already happened, but
		// actual object retirement remains in render-driven Gui::Clean. Keep this
		// operation in input until ownership and post-input visual rebinding agree.
		return (agui::gui->RmElement(&backing));
	};

	if (backing.selw->userDemo == SelectionWidget::NoDemoSelect) {
		backing.selw->ShowDemoList(demoSelectedCB);
		return;
	}
}

void LegacySelectMenuInput::SelectSave()
{
	auto& backing = ResolveMenu();
	//FIXME [SELECT-INPUT-02] This original reference-capturing callback now
	// crosses an extracted input operation. Reference capture does not retain
	// the GUI owner; resolve its lifetime at deferred callback invocation.
	// The exact deferred callback and mutations are retained below for review.
	const auto loadSelectedCB = [&](const std::string& userSave) {
		if (pregame != nullptr)
			return;

		backing.clientSetup->isHost = true;
		backing.clientSetup->saveFile = userSave;

		pregame = new CPreGame(backing.clientSetup);
		pregame->AsyncExecute(&CPreGame::LoadSaveFile, backing.clientSetup->saveFile);
		//pregame->LoadSaveFile(backing.clientSetup->saveFile);

		//FIXME [SELECT-INPUT-04] Mode replacement has already happened, but
		// actual object retirement remains in render-driven Gui::Clean. Keep this
		// operation in input until ownership and post-input visual rebinding agree.
		return (agui::gui->RmElement(&backing));
	};

	if (backing.selw->userLoad == SelectionWidget::NoSaveSelect) {
		backing.selw->ShowSavegameList(loadSelectedCB);
		return;
	}
}

void LegacySelectMenuInput::SelectLocalGame()
{
	auto& backing = ResolveMenu();
	if (backing.selw->userMod == SelectionWidget::NoModSelect) {
		backing.selw->ShowModList();
		return;
	}
	if (backing.selw->userMap == SelectionWidget::NoMapSelect) {
		backing.selw->ShowMapList();
		return;
	}
	if (backing.selw->userScript == SelectionWidget::NoScriptSelect) {
		backing.selw->ShowScriptList();
		return;
	}

	if (pregame == nullptr) {
		// in case of double-click
		if (backing.selw->userScript == SelectionWidget::SandboxAI)
			backing.selw->userScript.clear();

		pregame = new CPreGame(backing.clientSetup);
		pregame->AsyncExecute(&CPreGame::LoadSetupScript, StartScriptGen::CreateDefaultSetup(backing.selw->userMap, backing.selw->userMod, backing.selw->userScript, backing.clientSetup->myPlayerName));
		//pregame->LoadSetupScript(StartScriptGen::CreateDefaultSetup(backing.selw->userMap, backing.selw->userMod, backing.selw->userScript, backing.clientSetup->myPlayerName));
		//FIXME [SELECT-INPUT-04] Mode replacement has already happened, but
		// actual object retirement remains in render-driven Gui::Clean. Keep this
		// operation in input until ownership and post-input visual rebinding agree.
		return (agui::gui->RmElement(&backing));
	}
}

void LegacySelectMenuInput::RequestExit()
{
	auto& backing = ResolveMenu();
	gu->globalQuit = true;
	//FIXME [SELECT-INPUT-04] Mode replacement has already happened, but
	// actual object retirement remains in render-driven Gui::Clean. Keep this
	// operation in input until ownership and post-input visual rebinding agree.
	return (agui::gui->RmElement(&backing));
}

void LegacySelectMenuInput::ShowConnection(bool show)
{
	auto& backing = ResolveMenu();
	if (show && !backing.conWindow)
	{
		backing.conWindow = new ConnectWindow();
		backing.conWindow->Connect = (std::bind(&SelectMenu::DirectConnect, &backing, std::placeholders::_1));
		backing.conWindow->WantClose = std::bind(&SelectMenu::ShowConnectWindow, &backing, false);
	}
	else if (!show && backing.conWindow)
	{
		agui::gui->RmElement(backing.conWindow);
		backing.conWindow = nullptr;
	}
}

void LegacySelectMenuInput::ShowSettings(bool show, std::string name)
{
	auto& backing = ResolveMenu();
	if (show) {
		if (backing.settingsWindow) {
			agui::gui->RmElement(backing.settingsWindow);
			backing.settingsWindow = nullptr;
		}
		backing.settingsWindow = new SettingsWindow(name);
		backing.settingsWindow->OK = std::bind(&SelectMenu::ShowSettingsWindow, &backing, false, std::placeholders::_1);
		backing.settingsWindow->WantClose = std::bind(&SelectMenu::ShowSettingsWindow, &backing, false, "");
	}
	else if (!show && backing.settingsWindow) {
		agui::gui->RmElement(backing.settingsWindow);
		backing.settingsWindow = nullptr;
		const size_t p = name.find(" = ");
		if (p != std::string::npos) {
			configHandler->SetString(name.substr(0, p), name.substr(p + 3));
			backing.ShowSettingsList();
		}
		if (backing.curSelect != nullptr)
			backing.curSelect->list->SetFocus(true);
	}
}

void LegacySelectMenuInput::ListSettings()
{
	auto& backing = ResolveMenu();
	if (backing.curSelect == nullptr) {
		backing.curSelect = new ListSelectWnd("Select setting");
		backing.curSelect->Selected = std::bind(&SelectMenu::SelectSetting, &backing, std::placeholders::_1);
		backing.curSelect->WantClose = std::bind(&SelectMenu::CleanWindow, &backing);
	}
	backing.curSelect->list->RemoveAllItems();

	typedef std::map<std::string, std::string, doj::alphanum_less<std::string> > DataSorted;
	const std::map<std::string, std::string>& data = configHandler->GetData();
	const DataSorted dataSorted(data.begin(), data.end());

	for (const auto& item: dataSorted)
		backing.curSelect->list->AddItem(item.first + " = " + item.second, "");

	if (data.find(backing.userSetting) != data.end())
		backing.curSelect->list->SetCurrentItem(backing.userSetting + " = " + configHandler->GetString(backing.userSetting));

	backing.curSelect->list->RefreshQuery();
}

void LegacySelectMenuInput::SelectSetting(std::string setting)
{
	auto& backing = ResolveMenu();
	size_t p = setting.find(" = ");
	if(p != std::string::npos)
		setting = setting.substr(0, p);
	backing.userSetting = setting;
	configHandler->SetString("LastSelectedSetting", backing.userSetting);
	backing.ShowSettingsWindow(true, backing.userSetting);
}

void LegacySelectMenuInput::CloseSettingsList()
{
	auto& backing = ResolveMenu();
	if (backing.curSelect) {
		backing.ShowSettingsWindow(false, "");
		agui::gui->RmElement(backing.curSelect);
		backing.curSelect = nullptr;
	}
}

void LegacySelectMenuInput::ConnectToServer(const std::string& addr)
{
	auto& backing = ResolveMenu();
	configHandler->SetString("address", addr);

	backing.clientSetup->hostIP = addr;
	backing.clientSetup->isHost = false;

	//FIXME [SELECT-INPUT-03] Unlike local/demo/save actions this exact path
	// has no duplicate-start guard. Do not normalize it during extraction.
	// Test repeated input while the old GUI still awaits deferred removal.
	pregame = new CPreGame(backing.clientSetup);
	//FIXME [SELECT-INPUT-04] Mode replacement has already happened, but
	// actual object retirement remains in render-driven Gui::Clean. Keep this
	// operation in input until ownership and post-input visual rebinding agree.
	return (agui::gui->RmElement(&backing));
}

bool LegacySelectMenuInput::HandleEvent(const SDL_Event& ev)
{
	auto& backing = ResolveMenu();
	switch (ev.type) {
		case SDL_KEYDOWN: {
			if (ev.key.keysym.sym == SDLK_ESCAPE) {
				LOG("[SelectMenu] user exited");
				backing.Quit();
			} else if (ev.key.keysym.sym == SDLK_RETURN) {
				backing.Single();
				return true;
			}
			break;
		}
	}
	return false;
}

// [ activation / retirement ] GUI state remains on SelectMenu.
//FIXME [SELECT-INPUT-05] Construction currently initializes the base GUI and
// clientSetup/window members before this body. Move only the body into activation
// after those original initializers; do not construct a second GUI tree. Existing
// ConnectWindow/SettingsWindow definitions must become shared once, not copied.
void LegacySelectMenuInput::BuildMenuInterface()
{
	auto& backing = ResolveMenu();
	backing.SetPos(0, 0);
	backing.SetSize(1, 1);
	agui::gui->AddElement(&backing, true);

	{ // GUI stuff
		agui::Picture* background = new agui::Picture(&backing);

		{
			// can not conflict with LuaMenu archive, just keep in VFS if it was not already
			//FIXME [SELECT-INPUT-06] Archive changes and background RNG/asset loading
			// belong to activation, but currently need the graphics context.
			// Input-only activation cannot move off-thread with this body intact.
			vfsHandler->SetName("SelMenuVFS");
			vfsHandler->AddArchiveIf(configHandler->GetString("MenuArchive"), false);
			vfsHandler->SetName("SpringVFS");

			//TODO: select by resolution / aspect ratio with fallback image
			const std::vector<std::string> files = CFileHandler::FindFiles("bitmaps/ui/background/", "*");

			if (!files.empty())
				background->Load(files[ guRNG.NextInt(files.size()) ]);
		}

		backing.selw = new SelectionWidget(&backing);
		agui::VerticalLayout* menu = new agui::VerticalLayout(&backing);
		menu->backing.SetPos(0.1, 0.5);
		menu->backing.SetSize(0.4, 0.4);
		menu->SetBorder(1.2f);
		/*agui::TextElement* title = */new agui::TextElement("Recoil " + SpringVersion::GetFull(), menu); // will be deleted in menu
		Button* testGame = new Button("Test Game", menu);
		testGame->Clicked = std::bind(&SelectMenu::Single, &backing);

		Button* playDemo = new Button("Play Demo", menu);
		playDemo->Clicked = std::bind(&SelectMenu::Demo, &backing);

		Button* loadGame = new Button("Load Game", menu);
		loadGame->Clicked = std::bind(&SelectMenu::Load, &backing);

		backing.userSetting = configHandler->GetString("LastSelectedSetting");
		Button* editsettings = new Button("Edit Settings", menu);
		editsettings->Clicked = std::bind(&SelectMenu::ShowSettingsList, &backing);

		Button* directConnect = new Button("Direct Connect", menu);
		directConnect->Clicked = std::bind(&SelectMenu::ShowConnectWindow, &backing, true);

		Button* quit = new Button("Quit", menu);
		quit->Clicked = std::bind(&SelectMenu::Quit, &backing);
		background->GeometryChange();
	}

	backing.ShowConnectWindow(!backing.clientSetup->isHost);
}

void LegacySelectMenuInput::RetireMenuInterface()
{
	auto& backing = ResolveMenu();
	backing.ShowConnectWindow(false);
	backing.ShowSettingsWindow(false, "");
	backing.CleanWindow();
}
}
#endif
