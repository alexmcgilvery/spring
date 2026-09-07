/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "Present.h"
#include "Rendering/GlobalRendering.h"
#include "SystemNext/Diagnostics/LegacyRuntimeDiagnostics.h"
namespace runtime {
void PresentWindow(bool allowSwap)
{
	legacy::PhaseScope phase(Phase::Swap);
	globalRendering->SwapBuffers(allowSwap, false);
}
}
