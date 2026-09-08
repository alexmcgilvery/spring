/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../InvocationContext.h"

#include <filesystem>
#include <string>

namespace runtime {

// Forward declarations: application-owned diagnostic storage and output.
struct DiagnosticBuffer;
struct DiagnosticOutput;

struct DiagnosticObservationContext {
public:
	const InvocationContext& invocation;
	DiagnosticBuffer& output;
	std::string concern;
};

struct DiagnosticReportContext {
public:
	const InvocationContext& invocation;
	DiagnosticBuffer& records;
	DiagnosticOutput& output;
	const std::filesystem::path& writeDirectory;
	bool terminating;
};

} // namespace runtime
