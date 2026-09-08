/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "DiagnosticsContext.h"

namespace runtime {

/*
 * Cross-mode runtime observation and diagnostic-output outline. It does not contain the
 * Game-local semantic event journal or executable logging infrastructure.
 */
class Diagnostics {
public:
	void Observe(const DiagnosticObservationContext& context);
	void Report(const DiagnosticReportContext& context);
};

} // namespace runtime
