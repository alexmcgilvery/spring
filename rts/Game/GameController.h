/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_CONTROLLER_H_
#define _GAME_CONTROLLER_H_

#include <string>

#include "ConsoleHistory.h"
#include "GameControllerTextInput.h"

class CInputReceiver;

class CGameController
{
public:
	virtual ~CGameController();

	// CONCERN: render
	virtual bool Draw() { return true; }
	// CONCERN: session
	virtual bool Update() { return true; }
	// CONCERN: input
	virtual int KeyPressed(int keyCode, int scanCode, bool isRepeat) { return 0; }
	// CONCERN: input
	virtual int KeyMapChanged() { return 0; }
	// CONCERN: input
	virtual int KeyReleased(int keyCode, int scanCode) { return 0; }
	// CONCERN: input
	virtual int TextInput(const std::string& utf8Text) { return 0; }
	// CONCERN: input
	virtual int TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length) { return 0; }
	// CONCERN: input
	virtual void ResizeEvent() {}
	// CONCERN: input
	virtual bool MousePress(int x, int y, int button) { return 0; }
	// CONCERN: input
	virtual bool MouseRelease(int x, int y, int button) { return 0; }
	virtual CInputReceiver* GetInputReceiver() { return nullptr; }

};

extern CGameController* activeController;

#endif // _GAME_CONTROLLER_H_
