/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LoopPhaseScope.h"

#include "SystemNext/LoopServices.h"

namespace runtime {
LoopPhaseScope::LoopPhaseScope(ILoopDiagnostics& diagnostics, Phase phase) noexcept:
	diagnostics(diagnostics), token(diagnostics.BeginPhase(phase))
{}

LoopPhaseScope::~LoopPhaseScope()
{
	diagnostics.EndPhase(token);
}
}
