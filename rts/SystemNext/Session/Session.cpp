/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Session.h"
#include "IRuntimeMode.h"

namespace runtime {
SessionUpdate Session::AdvanceMode(IRuntimeMode* mode)
{
	if (mode == nullptr || !mode->HandlesSession())
		return SessionUpdate::NoSession();
	return mode->UpdateSession(*this);
}
}
