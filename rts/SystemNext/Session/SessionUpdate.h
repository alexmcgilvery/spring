/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SystemNext/Modes/FlowResult.h"

namespace runtime {

enum class ApplicationStatus { Continue, ExitRequested, Blocked };

/** Session continuation and explicit failure; display placement belongs to the mode. */
struct SessionUpdate {
	ApplicationStatus applicationStatus;
	BlockedFlow blocked;
	static SessionUpdate NoSession();
	static SessionUpdate Incomplete(std::string_view id, std::string_view reason);
	static SessionUpdate FromContinuation(bool continueRunning);
};
}
