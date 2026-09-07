/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Menu/LuaMenuController.h"

#include "Game/GlobalUnsynced.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaMenu.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"


#include "LuaMenuMode.h"

namespace runtime {
bool LuaMenuMode::HandlesSession() const { return false; }
DisplayPhase LuaMenuMode::GetDisplayPhase() const { return DisplayPhase::BeforeGraphics; }
bool LuaMenuMode::Reset()
{
	auto& backing = *static_cast<CLuaMenuController*>(Controller());
	if (!backing.Valid()) {
		// if no LuaMenu, cursor will not be updated (again) until game exists so force a reset
		// calling ReloadCursors here is not possible since no archives are loaded at this point
		mouse->ResetCursor();
		return false;
	}

	LOG("[LuaMenuController::%s] using menu archive \"%s\"", __func__, backing.menuArchive.c_str());

	// lock should not be needed here, but does no harm either
	vfsHandler->GrabLock();
	vfsHandler->SetName("LuaMenuVFS");
	vfsHandler->AddArchiveWithDeps(backing.menuArchive, false);
	vfsHandler->SetName("SpringVFS");
	vfsHandler->FreeLock();

	mouse->ReloadCursors();
	return true;
}
bool LuaMenuMode::Activate(const std::string& msg)
{
	auto& backing = *static_cast<CLuaMenuController*>(Controller());
	LOG("[LuaMenuController::%s(msg=\"%s\")] luaMenu=%p", __func__, msg.c_str(), luaMenu);

	// LuaMenu might have failed to load, making the controller deadweight
	if (luaMenu == nullptr)
		return false;

	assert(backing.Valid());
	SetActiveController(luaMenuController);

	mouse->ShowMouse();
	luaMenu->ActivateMenu(msg);
	return true;
}
void LuaMenuMode::ResizeEvent()
{
	eventHandler.ViewResize();
}
int LuaMenuMode::KeyReleased(int keyCode, int scanCode)
{
	luaInputReceiver->KeyReleased(keyCode, scanCode);
	return 0;
}
int LuaMenuMode::KeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	luaInputReceiver->KeyPressed(keyCode, scanCode, isRepeat);
	return 0;
}
int LuaMenuMode::TextInput(const std::string& utf8Text)
{
	eventHandler.TextInput(utf8Text);
	return 0;
}
int LuaMenuMode::TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length)
{
	eventHandler.TextEditing(utf8Text, start, length);
	return 0;
}
ApplicationStatus LuaMenuMode::UpdateDisplay(ModeFrame&)
{
	ZoneScoped;

	// we should not become the active controller unless this holds (see ::Activate)
	assert(luaMenu != nullptr);

	eventHandler.CollectGarbage(false);
	infoConsole->PushNewLinesToEventHandler();
	mouse->Update();
	mouse->UpdateCursors();
	eventHandler.Update();
	// calls IsAbove
	mouse->GetCurrentTooltip();

	return ApplicationStatus::Continue;
}
RenderResult LuaMenuMode::Render(ModeFrame&)
{
	auto& backing = *static_cast<CLuaMenuController*>(Controller());
	// we should not become the active controller unless this holds (see ::Activate)
	assert(luaMenu != nullptr);

	// render if global rendering active + luamenu allows it, and at least once per 30s
	const bool allowDraw = (globalRendering->active && luaMenu->AllowDraw());
	const bool forceDraw = ((spring_gettime() - backing.lastDrawFrameTime).toSecsi() > 30);

	if (allowDraw || forceDraw) {
		globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
		ClearScreen();

		eventHandler.DrawGenesis();
		eventHandler.DrawScreen();
		mouse->DrawCursor();
		eventHandler.DrawScreenPost();

		backing.lastDrawFrameTime = spring_gettime();
		return RenderResult::Ready();
	}

	spring_msecs(10).sleep(true); // no draw needed, sleep a bit
	return RenderResult::Skipped();
}
}
