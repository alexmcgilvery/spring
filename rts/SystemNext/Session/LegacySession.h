/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Session.h"
#include "IRuntimeMode.h"

namespace runtime::legacy {
/** Resolve the current game/loading controller on every call; never retain it. */
class LegacySession final : public Session {
public:
	SessionUpdate Advance() override;
};

/** Bridge current mode selection without storing session-owned controller pointers. */
class LegacyRuntimeMode final : public IRuntimeMode {
public:
	bool HandlesSession() const override;
	SessionUpdate UpdateSession(Session& session) override;
};

/** Invoke and observe exactly one existing controller update, including null. */
bool UpdateCurrentController();
}
