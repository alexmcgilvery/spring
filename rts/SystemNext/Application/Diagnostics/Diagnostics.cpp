/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Diagnostics.h"

namespace runtime {

Diagnostics::~Diagnostics() = default;

// TODO(SystemNext): Add the concrete application diagnostics implementation.
// It must accept bounded observations from loop/stage boundaries, serialize them
// only at Report(), use application-owned output resources, and never expose a
// writable diagnostics service to modes. This translation unit currently defines
// only the abstract ownership boundary; no diagnostic records are produced.

} // namespace runtime
