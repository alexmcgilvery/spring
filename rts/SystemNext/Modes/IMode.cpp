/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IMode.h"

namespace runtime {

IMode::IMode(ModeKind kind)
	: kind(kind)
{
}

IMode::~IMode() = default;

} // namespace runtime
