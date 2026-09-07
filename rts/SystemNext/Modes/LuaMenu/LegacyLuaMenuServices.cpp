/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyLuaMenuServices.h"

//FIXME [LUA-FLOW-01] The complete backing statements remain in this disabled
// draft while private access/binding is unresolved. The existing Mode/Visuals
// compose these operations; none forwards the controller's whole Update/Draw.
#if 0
#include "Game/GlobalUnsynced.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaMenu.h"
#include "System/EventHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Log/ILog.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"

namespace runtime {
//FIXME [LUA-FLOW-06] CLuaMenuController::Update owns this lexical profiler
// scope across every client operation. The inactive mode interface currently
// omits it. Add the scoped call at that boundary before activation; invoking
// this macro in an operation that returns immediately would shorten the scope.
void LegacyLuaMenuServices::WithClientUpdate(const std::function<void()>& body)
{
	ZoneScoped;
	body();
}

// [ display ] Existing client maintenance precedes render eligibility.
void LegacyLuaMenuServices::RequireActiveLuaHandler()
{
	assert(luaMenu != nullptr);
}

void LegacyLuaMenuServices::CollectScriptGarbage()
{
	eventHandler.CollectGarbage(false);
}

void LegacyLuaMenuServices::PublishConsoleLines()
{
	infoConsole->PushNewLinesToEventHandler();
}

void LegacyLuaMenuServices::UpdateMouse()
{
	mouse->Update();
}

void LegacyLuaMenuServices::UpdateCursors()
{
	mouse->UpdateCursors();
}

void LegacyLuaMenuServices::DispatchClientUpdate()
{
	//FIXME [LUA-FLOW-02] This display callback dispatches synchronous Lua that can
	// submit commands or request lifecycle changes. Moving it to a render worker
	// changes both thread ownership and command order. Keep the call here until
	// the input/session boundary has callback-order and replacement evidence.
	eventHandler.Update();
}

void LegacyLuaMenuServices::EvaluateTooltip()
{
	mouse->GetCurrentTooltip();
}

bool LegacyLuaMenuServices::IsWindowActive() const
{
	return globalRendering->active;
}

bool LegacyLuaMenuServices::RequestScriptDrawPermission()
{
	return luaMenu->AllowDraw();
}

std::int64_t LegacyLuaMenuServices::WholeSecondsSinceLastCompletedDraw()
{
	//FIXME [LUA-FLOW-03] The timestamp belongs to the surviving controller.
	// ResolveController is deliberately unimplemented until mode binding handles
	// reset/replacement. Do not create a second timestamp or cache the controller
	// across AllowDraw. Retain this exact elapsed calculation at eligibility.
	return (spring_gettime() - ResolveController().lastDrawFrameTime).toSecsi();
}

void LegacyLuaMenuServices::SleepForSkippedDraw()
{
	spring_msecs(10).sleep(true);
}

void LegacyLuaMenuServices::AdvanceDrawCounter()
{
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
}

void LegacyLuaMenuServices::ClearScreen()
{
	::ClearScreen();
}

void LegacyLuaMenuServices::DispatchDrawGenesis()
{
	eventHandler.DrawGenesis();
}

void LegacyLuaMenuServices::DispatchDrawScreen()
{
	eventHandler.DrawScreen();
}

void LegacyLuaMenuServices::DrawCursor()
{
	mouse->DrawCursor();
}

void LegacyLuaMenuServices::DispatchDrawScreenPost()
{
	eventHandler.DrawScreenPost();
}

void LegacyLuaMenuServices::RecordCompletedDrawTime()
{
	ResolveController().lastDrawFrameTime = spring_gettime();
}

// [ input ] No second input poll: these are the existing event entry bodies.
//FIXME [LUA-FLOW-04] Key/text/resize callbacks invoke Lua synchronously. Their
// effects can replace the mode before display/render. Wire them through the
// current input dispatcher, re-resolve visuals after input, and test resize GL
// ownership. Copying these methods does not authorize dispatching input twice.
int LegacyLuaMenuServices::KeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	luaInputReceiver->KeyPressed(keyCode, scanCode, isRepeat);
	return 0;
}

int LegacyLuaMenuServices::KeyReleased(int keyCode, int scanCode)
{
	luaInputReceiver->KeyReleased(keyCode, scanCode);
	return 0;
}

int LegacyLuaMenuServices::TextInput(const std::string& utf8Text)
{
	eventHandler.TextInput(utf8Text);
	return 0;
}

int LegacyLuaMenuServices::TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length)
{
	eventHandler.TextEditing(utf8Text, start, length);
	return 0;
}

void LegacyLuaMenuServices::ResizeEvent()
{
	eventHandler.ViewResize();
}

// [ lifecycle ] State remains on the existing controller.
//FIXME [LUA-FLOW-05] These exact lock/callback statements can fail or trigger
// lifecycle work. Retaining a backing reference across them needs a proved
// lifetime contract. Preserve their placement; do not silently add lock cleanup
// or turn Activate into a per-frame display step.
bool LegacyLuaMenuServices::ResetContent()
{
	auto& backing = ResolveController();
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

bool LegacyLuaMenuServices::Activate(const std::string& msg)
{
	auto& backing = ResolveController();
	LOG("[LuaMenuController::%s(msg=\"%s\")] luaMenu=%p", __func__, msg.c_str(), luaMenu);

	// LuaMenu might have failed to load, making the controller deadweight
	if (luaMenu == nullptr)
		return false;

	assert(backing.Valid());
	activeController = luaMenuController;

	mouse->ShowMouse();
	luaMenu->ActivateMenu(msg);
	return true;
}

}
#endif
