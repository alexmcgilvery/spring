/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../InvocationContext.h"
#include <filesystem>
#include <string_view>
namespace runtime {
// Payload/buffer interfaces are intentionally incomplete; no hidden implementation.
struct DiagnosticBuffer;
struct DiagnosticOutput;
struct DiagnosticObservationContext {
	const InvocationContext& invocation;
	DiagnosticBuffer& output;
	std::string_view concern;
};
struct DiagnosticReportContext {
	const InvocationContext& invocation;
	DiagnosticBuffer& records;
	DiagnosticOutput& output;
	const std::filesystem::path& writeDirectory;
	bool terminating;
};
/* Text/path views are borrowed only for this call. Retained records must own
 * their payloads. Report owns no engine log stream through this context. */
}
