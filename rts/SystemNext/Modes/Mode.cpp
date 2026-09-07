/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Mode.h"

namespace runtime {
void Mode::BindController(CGameController* value) { controller = value; }
CGameController* Mode::Controller() const
{
	if (controller == nullptr)
		throw IncompleteFlow("MODE-NO-BACKING", "The requested mode has no live backing controller");
	return controller;
}
SessionUpdate Mode::UpdateSession(Session&)
{
	return SessionUpdate::Incomplete("MODE-NO-SESSION", "Scheduler invoked an absent session concern");
}
ApplicationStatus Mode::UpdateDisplay(ModeFrame& frame)
{
	return frame.Block("MODE-NO-DISPLAY", "Scheduler invoked an absent display concern");
}
int Mode::KeyPressed(int, int, bool) { return 0; }
int Mode::KeyMapChanged() { return 0; }
int Mode::KeyReleased(int, int) { return 0; }
int Mode::TextInput(const std::string&) { return 0; }
int Mode::TextEditing(const std::string&, unsigned int, unsigned int) { return 0; }
void Mode::ResizeEvent() {}
bool Mode::MousePress(int, int, int) { return false; }
bool Mode::MouseRelease(int, int, int) { return false; }
CInputReceiver* Mode::GetInputReceiver() { return nullptr; }
}
