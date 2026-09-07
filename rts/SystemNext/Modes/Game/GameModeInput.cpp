/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameMode.h"

#include <Rml/Backends/RmlUi_Backend.h>
#include "Game/Game.h"
#include "Game/GameControllerTextInput.h"
#include "Game/ConsoleHistory.h"
#include "Game/InMapDraw.h"
#include "Game/UI/MiniMap.h"
#include "Rendering/Env/IWater.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Misc/TracyDefs.h"
#include "System/TimeProfiler.h"

namespace runtime {
void GameMode::ResizeEvent()
{
	LOG("[Game::%s][1]", __func__);

	{
		SCOPED_ONCE_TIMER("Game::ViewResize")

		if (minimap != nullptr)
			minimap->UpdateGeometry();

		//recreate water on resize (lazy but works)
		const auto wt = IWater::GetWater()->GetID();
		IWater::KillWater();
		IWater::SetWater(wt);
	}

	LOG("[Game::%s][2]", __func__);

	{
		SCOPED_ONCE_TIMER("EventHandler::ViewResize");

		gameTextInput.ViewResize();
		eventHandler.ViewResize();
	}
}

int GameMode::KeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	auto& backing = *static_cast<CGame*>(Controller());
	backing.gameInputReceiver.KeyPressed(keyCode, scanCode, isRepeat);
	return 0;
}

int GameMode::KeyReleased(int keyCode, int scanCode)
{
	auto& backing = *static_cast<CGame*>(Controller());
	backing.gameInputReceiver.KeyReleased(keyCode, scanCode);
	return 0;
}

CInputReceiver* GameMode::GetInputReceiver()
{
	return &static_cast<CGame*>(Controller())->gameInputReceiver;
}

int GameMode::KeyMapChanged()
{
	RECOIL_DETAILED_TRACY_ZONE;
	eventHandler.KeyMapChanged();

	return 0;
}

int GameMode::TextInput(const std::string& utf8Text)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (RmlGui::ProcessTextInput(utf8Text))
		return 0;

	if (eventHandler.TextInput(utf8Text))
		return 0;

	return (gameTextInput.SetInputText(utf8Text));
}

int GameMode::TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (eventHandler.TextEditing(utf8Text, start, length))
		return 0;

	return (gameTextInput.SetEditText(utf8Text));
}



/** Submit at the original display point; collecting platform input is separate. */
void GameMode::SubmitPendingInput()
{
	auto& backing = *static_cast<CGame*>(Controller());
	if (gameTextInput.SendPromptInput()) {
		gameConsoleHistory.AddLine(gameTextInput.userInput);
		backing.SendNetChat(gameTextInput.userInput);
		gameTextInput.ClearInput();
	}
	if (inMapDrawer->IsWantLabel() && gameTextInput.SendLabelInput())
		gameTextInput.ClearInput();

}
}
