/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../InvocationContext.h"
class CGlobalRendering;
namespace runtime {
/**
 * Borrow the existing rendering/window state. The caller must already hold the
 * required context and synchronization on the calling thread. This reference is
 * not a lock, ownership token, or permission to make cross-thread graphics calls.
 */
struct GraphicsAccess { CGlobalRendering& rendering; };
enum class PresentationOrigin { OrdinaryIteration, LoadingProgress };
struct GraphicsSynchronizationContext {
	const InvocationContext& invocation;
	CGlobalRendering& rendering;
	bool loadingUsesThreadSafety;
};
struct PresentContext {
	const InvocationContext& invocation;
	GraphicsAccess& graphics;
	PresentationOrigin origin;
	bool allowSwap;
};
}
