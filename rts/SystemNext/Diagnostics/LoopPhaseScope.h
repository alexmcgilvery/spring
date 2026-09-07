/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "PhaseToken.h"

namespace runtime {
class ILoopDiagnostics;
/** Balance observation on normal return and engine exception unwinding. */
class LoopPhaseScope {
public:
	LoopPhaseScope(ILoopDiagnostics& diagnostics, Phase phase) noexcept;
	~LoopPhaseScope();
	LoopPhaseScope(const LoopPhaseScope&) = delete;
	LoopPhaseScope& operator=(const LoopPhaseScope&) = delete;
private:
	ILoopDiagnostics& diagnostics;
	PhaseToken token;
};
}
