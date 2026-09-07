/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SessionUpdate.h"

namespace runtime {
SessionUpdate SessionUpdate::Incomplete(std::string_view id, std::string_view reason)
{
	return {ApplicationStatus::Blocked, {std::string(id), std::string(reason)}};
}

SessionUpdate SessionUpdate::NoSession()
{
	return {ApplicationStatus::Continue, {}};
}

SessionUpdate SessionUpdate::FromContinuation(bool continueRunning)
{
	return continueRunning
		? SessionUpdate{ApplicationStatus::Continue, {}}
		: SessionUpdate{ApplicationStatus::ExitRequested, {}};
}
}
