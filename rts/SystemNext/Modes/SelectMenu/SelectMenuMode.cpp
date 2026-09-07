/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Menu/SelectMenu.h"

#include <SDL_keycode.h>
#include <functional>
#include <sstream>
#include <stack>

#include "Menu/SelectionWidget.h"
#include "System/AIScriptHandler.h"
#include "Game/ClientSetup.h"
#include "Game/GameVersion.h"
#include "Game/GlobalUnsynced.h"
#include "Game/PreGame.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Input/InputHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/MsgStrings.h"
#include "System/StartScriptGen.h"
#include "Rendering/GlobalRendering.h"
#include "aGui/Gui.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/Button.h"
#include "aGui/LineEdit.h"
#include "aGui/TextElement.h"
#include "aGui/Window.h"
#include "aGui/Picture.h"
#include "aGui/List.h"
#include "Menu/alphanum.hpp"

using std::string;
using agui::Button;
using agui::HorizontalLayout;



#include "SelectMenuMode.h"

class ConnectWindow : public agui::Window {
public:
	ConnectWindow() : agui::Window("Connect to server") {
		agui::gui->AddElement(this);
		SetPos(0.5, 0.5);
		SetSize(0.4, 0.2);

		agui::VerticalLayout* wndLayout = new agui::VerticalLayout(this);
		HorizontalLayout* input = new HorizontalLayout(wndLayout);
		/*agui::TextElement* label = */new agui::TextElement("Address:", input); // will be deleted in input
		address = new agui::LineEdit(input);
		address->DefaultAction = std::bind(&ConnectWindow::Finish, this, true);
		address->SetFocus(true);
		address->SetContent(configHandler->GetString("address"));
		HorizontalLayout* buttons = new HorizontalLayout(wndLayout);
		Button* connect = new Button("Connect", buttons);
		connect->Clicked = std::bind(&ConnectWindow::Finish, this, true);
		Button* close = new Button("Close", buttons);
		close->Clicked = std::bind(&ConnectWindow::Finish, this, false);
		GeometryChange();
	}

	OnClickStringType Connect;
	agui::LineEdit* address;

private:
	void Finish(bool connect) {
		if (connect)
			Connect(address->GetContent());
		else
			WantClose();
	};
};

class SettingsWindow : public agui::Window {
public:
	SettingsWindow(std::string &name) : agui::Window(name) {
		agui::gui->AddElement(this);
		SetPos(0.5, 0.5);
		SetSize(0.4, 0.2);

		agui::VerticalLayout* wndLayout = new agui::VerticalLayout(this);
		HorizontalLayout* input = new HorizontalLayout(wndLayout);
		/*agui::TextElement* value_label = */new agui::TextElement("Value:", input); // will be deleted in input
		value = new agui::LineEdit(input);
		value->DefaultAction = std::bind(&SettingsWindow::Finish, this, true);
		value->SetFocus(true);
		if (configHandler->IsSet(name))
			value->SetContent(configHandler->GetString(name));
		HorizontalLayout* buttons = new HorizontalLayout(wndLayout);
		Button* ok = new Button("OK", buttons);
		ok->Clicked = std::bind(&SettingsWindow::Finish, this, true);
		Button* close = new Button("Cancel", buttons);
		close->Clicked = std::bind(&SettingsWindow::Finish, this, false);
		GeometryChange();
	}

	OnClickStringType OK;
	agui::LineEdit* value;

private:
	void Finish(bool set) {
		if (set)
			OK(title + " = " + value->GetContent());
		else
			WantClose();
	};
};





namespace runtime {
bool SelectMenuMode::HandlesSession() const { return false; }
DisplayPhase SelectMenuMode::GetDisplayPhase() const { return DisplayPhase::None; }
void SelectMenuMode::Initialize()
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
	backing.SetPos(0, 0);
	backing.SetSize(1, 1);
	agui::gui->AddElement(&backing, true);

	{ // GUI stuff
		agui::Picture* background = new agui::Picture(&backing);

		{
			// can not conflict with LuaMenu archive, just keep in VFS if it was not already
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
		menu->SetPos(0.1, 0.5);
		menu->SetSize(0.4, 0.4);
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

	ShowConnectWindow(!backing.clientSetup->isHost);
}
void SelectMenuMode::Retire()
{
	ShowConnectWindow(false);
	ShowSettingsWindow(false, "");
	CleanWindow();
}
RenderResult SelectMenuMode::Render(ModeFrame&)
{
	spring_msecs(10).sleep(true);
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
	ClearScreen();
	agui::gui->Draw();

	return RenderResult::Ready();
}
void SelectMenuMode::Demo()
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
	const auto demoSelectedCB = [&](const std::string& userDemo) {
		if (pregame != nullptr)
			return;

		backing.clientSetup->isHost = true;
		backing.clientSetup->myPlayerName += " (spec)";
		backing.clientSetup->demoFile = userDemo;

		pregame = new CPreGame(backing.clientSetup);
		pregame->AsyncExecute(&CPreGame::LoadDemoFile, backing.clientSetup->demoFile);
		//pregame->LoadDemoFile(backing.clientSetup->demoFile);

		return (agui::gui->RmElement(&backing));
	};

	if (backing.selw->userDemo == SelectionWidget::NoDemoSelect) {
		backing.selw->ShowDemoList(demoSelectedCB);
		return;
	}
}
void SelectMenuMode::Load()
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
	const auto loadSelectedCB = [&](const std::string& userSave) {
		if (pregame != nullptr)
			return;

		backing.clientSetup->isHost = true;
		backing.clientSetup->saveFile = userSave;

		pregame = new CPreGame(backing.clientSetup);
		pregame->AsyncExecute(&CPreGame::LoadSaveFile, backing.clientSetup->saveFile);
		//pregame->LoadSaveFile(backing.clientSetup->saveFile);

		return (agui::gui->RmElement(&backing));
	};

	if (backing.selw->userLoad == SelectionWidget::NoSaveSelect) {
		backing.selw->ShowSavegameList(loadSelectedCB);
		return;
	}
}
void SelectMenuMode::Single()
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
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
		return (agui::gui->RmElement(&backing));
	}
}
void SelectMenuMode::Quit()
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
	gu->globalQuit = true;
	return (agui::gui->RmElement(&backing));
}
void SelectMenuMode::ShowConnectWindow(bool show)
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
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
void SelectMenuMode::ShowSettingsWindow(bool show, std::string name)
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
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
			ShowSettingsList();
		}
		if (backing.curSelect != nullptr)
			backing.curSelect->list->SetFocus(true);
	}
}
void SelectMenuMode::ShowSettingsList()
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
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
void SelectMenuMode::SelectSetting(std::string setting)
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
	size_t p = setting.find(" = ");
	if(p != std::string::npos)
		setting = setting.substr(0, p);
	backing.userSetting = setting;
	configHandler->SetString("LastSelectedSetting", backing.userSetting);
	ShowSettingsWindow(true, backing.userSetting);
}
void SelectMenuMode::CleanWindow()
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
	if (backing.curSelect) {
		ShowSettingsWindow(false, "");
		agui::gui->RmElement(backing.curSelect);
		backing.curSelect = nullptr;
	}
}
void SelectMenuMode::DirectConnect(const std::string& addr)
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
	configHandler->SetString("address", addr);

	backing.clientSetup->hostIP = addr;
	backing.clientSetup->isHost = false;

	pregame = new CPreGame(backing.clientSetup);
	return (agui::gui->RmElement(&backing));
}
bool SelectMenuMode::HandleEventSelf(const SDL_Event& ev)
{
	auto& backing = *static_cast<SelectMenu*>(Controller());
	switch (ev.type) {
		case SDL_KEYDOWN: {
			if (ev.key.keysym.sym == SDLK_ESCAPE) {
				LOG("[SelectMenu] user exited");
				Quit();
			} else if (ev.key.keysym.sym == SDLK_RETURN) {
				Single();
				return true;
			}
			break;
		}
	}
	return false;
}
}
