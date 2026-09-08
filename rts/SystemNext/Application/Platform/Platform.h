/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "PlatformSnapshots.h"

namespace runtime {

/** Owns native event collection and window lifetime without exposing handles to modes. */
class Platform {
public:
	virtual ~Platform();

	virtual bool BeginIteration() = 0;
	virtual PlatformPublications CollectPublications() = 0;
	virtual bool ExitRequested() const = 0;
	virtual bool ReloadRequested() const = 0;
};

} // namespace runtime
