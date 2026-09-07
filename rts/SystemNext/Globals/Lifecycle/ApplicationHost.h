/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
#include "../InvocationContext.h"
#include "../Graphics/GraphicsContext.h"
#include "../Graphics/GraphicsScope.h"
#include <memory>
namespace runtime {
/**
 * Coarse application boundaries supplied to the loop, not per-statement services.
 * BeginIteration services lifecycle/platform maintenance and decides continuation.
 * CollectInput gathers/filters events; it must not also run the mode's Input block.
 * Reload/Initialize own resource ordering; ordinary exceptions propagate to the
 * caller, which remains responsible for emergency shutdown after failed execution.
 * These contracts have no engine implementation and do not redirect legacy input.
 */
class ApplicationHost {
public:
	virtual ~ApplicationHost();
	virtual void Initialize() = 0;
	virtual bool BeginIteration() = 0;
	virtual void CollectInput() = 0;
	virtual bool ReloadRequested() const = 0;
	virtual void Reload() = 0;
	virtual InvocationContext CaptureInvocation(ModeIdentity mode, std::uint64_t iteration) = 0;
	virtual std::unique_ptr<GraphicsScope> AcquireGraphics() = 0;
	virtual void Present(PresentationOrigin origin) = 0;
	virtual void FlushDiagnostics() = 0;
	virtual void Shutdown() = 0;
};
}
