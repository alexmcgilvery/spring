/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

//FIXME [LUA-FLOW-01] Disabled extraction draft: resolve private backing access
// and mode retirement before enabling these declarations and bodies. No copied
// controller state is introduced. Code below is preserved for review, not compiled
// or validated as an engine adapter. Each call has its actual body in the .cpp.
#if 0
#include <functional>
#include "ILuaMenuServices.h"
#include "Menu/LuaMenuController.h"

namespace runtime {
class LegacyLuaMenuServices final : public ILuaMenuServices {
public:
	void WithClientUpdate(const std::function<void()>& body);
	void RequireActiveLuaHandler() override;
	void CollectScriptGarbage() override;
	void PublishConsoleLines() override;
	void UpdateMouse() override;
	void UpdateCursors() override;
	void DispatchClientUpdate() override;
	void EvaluateTooltip() override;
	bool IsWindowActive() const override;
	bool RequestScriptDrawPermission() override;
	std::int64_t WholeSecondsSinceLastCompletedDraw() override;
	void SleepForSkippedDraw() override;
	void AdvanceDrawCounter() override;
	void ClearScreen() override;
	void DispatchDrawGenesis() override;
	void DispatchDrawScreen() override;
	void DrawCursor() override;
	void DispatchDrawScreenPost() override;
	void RecordCompletedDrawTime() override;

	// [ input ] Event callbacks remain on the existing application input route.
	int KeyPressed(int keyCode, int scanCode, bool isRepeat);
	int KeyReleased(int keyCode, int scanCode);
	int TextInput(const std::string& utf8Text);
	int TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length);
	void ResizeEvent();

	// [ lifecycle ] Content reset and activation are distinct from frame work.
	bool ResetContent();
	bool Activate(const std::string& msg);

private:
	CLuaMenuController& ResolveController();
};
}
#endif
