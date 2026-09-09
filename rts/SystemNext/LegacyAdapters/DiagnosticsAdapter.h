/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Application/Diagnostics/Diagnostics.h"

namespace runtime {

/**
 * Minimal concrete Diagnostics adapter.
 * Report is a no-op; the existing engine logging and profiling systems
 * continue to operate independently.
 */
class DiagnosticsAdapter final : public Diagnostics {
public:
	DiagnosticsAdapter() = default;
	~DiagnosticsAdapter() override = default;

	void Report() override;
};

} // namespace runtime
