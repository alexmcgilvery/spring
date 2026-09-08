/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ApplicationLifecycle.h"

namespace runtime {

ApplicationLifecycle::~ApplicationLifecycle() = default;

// TODO(SystemNext): Add the concrete application lifecycle implementation.
// It must construct initial and replacement modes, own initialization/reload/
// shutdown dependencies, and retire those dependencies only after snapshot
// acceptance closes. This translation unit currently defines only the abstract
// ownership boundary; no engine resources or modes are constructed here.

} // namespace runtime
