/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IMode.h"
#include <stdexcept>
namespace runtime {
IMode::IMode(ModeKind kind, bool handlesSession, DisplayPhase displayPhase):
	kind(kind), handlesSession(handlesSession), displayPhase(displayPhase)
{}
IMode::~IMode() = default;
void IMode::Session(const ModeSessionContext&)
{
	throw std::logic_error("Mode does not have a session concern");
}
void IMode::Display(const ModeDisplayContext&)
{
	throw std::logic_error("Mode does not have a display concern");
}
}
