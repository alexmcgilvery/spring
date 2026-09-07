/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Session/IRuntimeMode.h"
#include "SystemNext/Presentation/ModeFrame.h"

class CGameController;
class CInputReceiver;

namespace runtime {
/**
 * Own a controller's behavior, not a second copy of its state. The binding owns
 * mode objects independently of backing retirement. Resolve the backing at each
 * invocation; callbacks may retire it. Optional concerns are scheduled explicitly.
 */
class Mode : public IRuntimeMode {
public:
	virtual DisplayPhase GetDisplayPhase() const = 0;
	SessionUpdate UpdateSession(Session&) override;
	virtual ApplicationStatus UpdateDisplay(ModeFrame& frame);
	virtual RenderResult Render(ModeFrame& frame) = 0;
	void BindController(CGameController* controller);

	// Unhandled input is intentional; platform routing preserves consumption.
	virtual int KeyPressed(int keyCode, int scanCode, bool isRepeat);
	virtual int KeyMapChanged();
	virtual int KeyReleased(int keyCode, int scanCode);
	virtual int TextInput(const std::string& text);
	virtual int TextEditing(const std::string& text, unsigned int start, unsigned int length);
	virtual void ResizeEvent();
	virtual bool MousePress(int x, int y, int button);
	virtual bool MouseRelease(int x, int y, int button);
	virtual CInputReceiver* GetInputReceiver();
protected:
	CGameController* Controller() const;
private:
	CGameController* controller = nullptr;
};
}
