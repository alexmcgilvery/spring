/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "LegacyModeBinding.h"
#include "SelectMenu/SelectMenuMode.h"
#include "LuaMenu/LuaMenuMode.h"
#include "PreGame/PreGameMode.h"
#include "Loading/LoadingMode.h"
#include "Game/GameMode.h"
#include "SystemNext/Session/Session.h"
#include "Menu/SelectMenu.h"
#include "Menu/LuaMenuController.h"
#include "Game/PreGame.h"
#include "Game/LoadScreen.h"
#include "Game/Game.h"
namespace runtime {
SelectMenuMode& GetSelectMenuMode() { static SelectMenuMode mode; return mode; }
LuaMenuMode& GetLuaMenuMode() { static LuaMenuMode mode; return mode; }
PreGameMode& GetPreGameMode() { static PreGameMode mode; return mode; }
LoadingMode& GetLoadingMode() { static LoadingMode mode; return mode; }
GameMode& GetGameMode() { static GameMode mode; return mode; }
Session& GetRuntimeSession() { static Session session; return session; }
//FIXME ADAPTER-BINDING: This retained selection code assumes writable legacy
// backing and a controller generation API that legacy does not supply. Replace
// those dependencies with SystemNext-owned state/lifetime before activation.
// This translation unit is explicitly unavailable in sources.cmake.
Mode* ResolveActiveMode()
{
	Mode* mode = nullptr;
	if (activeController == nullptr)
		return nullptr;
	if (dynamic_cast<SelectMenu*>(activeController) != nullptr) mode = &GetSelectMenuMode();
	else if (dynamic_cast<CLuaMenuController*>(activeController) != nullptr) mode = &GetLuaMenuMode();
	else if (dynamic_cast<CPreGame*>(activeController) != nullptr) mode = &GetPreGameMode();
	else if (dynamic_cast<CLoadScreen*>(activeController) != nullptr) mode = &GetLoadingMode();
	else if (dynamic_cast<CGame*>(activeController) != nullptr) mode = &GetGameMode();
	else throw IncompleteFlow("MODE-UNKNOWN-CONTROLLER", "Controller has no owned mode mapping");
	mode->BindController(activeController);
	return mode;
}
Mode* LegacyModeBinding::Resolve() { return ResolveActiveMode(); }
std::uint64_t LegacyModeBinding::Generation() const { return GetActiveControllerGeneration(); }
}
