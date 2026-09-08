/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Platform.h"

namespace runtime {

Platform::~Platform() = default;

// TODO(SystemNext): Add the concrete platform implementation.
// It must collect each native event exactly once, publish input and window facts
// as owned values, and surface OS exit/reload requests without exposing native
// handles to modes. This translation unit currently defines only the abstract
// ownership boundary; no platform events or window state are collected here.

} // namespace runtime
