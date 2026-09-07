/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "IVisualFrame.h"

namespace runtime {
/**
 * Preserve the synchronous visual path while exposing its scheduling boundary.
 * Hooks are protected to keep engine bindings and deterministic test services
 * below the execution flow. Instances are used serially, never concurrently.
 */
class SerialVisualFrame : public IVisualFrame {
public:
	ApplicationStatus ExecuteFrame(const VisualFrameContext& context) final;
protected:
	virtual ApplicationStatus UpdateClientMode() = 0;
	virtual void LockDraw() = 0;
	virtual void UnlockDraw() noexcept = 0;
	virtual bool Draw() = 0;
	virtual void Present(bool allowSwap) = 0;
private:
	class DrawScope {
	public:
		explicit DrawScope(SerialVisualFrame& frame);
		~DrawScope();
		DrawScope(const DrawScope&) = delete;
		DrawScope& operator=(const DrawScope&) = delete;
	private:
		SerialVisualFrame& frame;
	};
	bool PrepareAndRender(const VisualFrameContext& context, ApplicationStatus clientStatus);
};
}
