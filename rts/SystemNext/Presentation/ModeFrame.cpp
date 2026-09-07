/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ModeFrame.h"

namespace runtime {
ModeFrameData::~ModeFrameData() = default;
ApplicationStatus ModeFrame::Block(std::string_view id, std::string_view reason)
{
	allowRender = false;
	blocked = {std::string(id), std::string(reason)};
	return ApplicationStatus::Blocked;
}
}
