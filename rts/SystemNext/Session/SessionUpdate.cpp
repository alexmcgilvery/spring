/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SessionUpdate.h"

namespace runtime {
SessionUpdate SessionUpdate::NoSession()
{
	return {ApplicationStatus::Continue, {SessionOutcome::NoSession}};
}

SessionUpdate SessionUpdate::FromContinuation(bool continueRunning)
{
	return continueRunning
		? SessionUpdate{ApplicationStatus::Continue, {SessionOutcome::Continue}}
		: SessionUpdate{ApplicationStatus::ExitRequested, {SessionOutcome::ExitRequested}};
}
}
