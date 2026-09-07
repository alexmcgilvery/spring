/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "BoundedTraceBuffer.h"

namespace Json { class Value; }

namespace runtime {

// Called only at outer boundaries. The legacy FileSink retains FILE ownership.
class JsonLinesTraceWriter {
public:
	JsonLinesTraceWriter(FILE* stream, std::size_t byteLimit): stream(stream), byteLimit(byteLimit) {}
	bool Start(Json::Value header) noexcept;
	bool Drain(BoundedTraceBuffer& buffer) noexcept;
	bool Finish(const BoundedTraceBuffer& buffer, bool complete) noexcept;
	bool Failed() const noexcept { return failed; }
	std::size_t Bytes() const noexcept { return bytes; }

private:
	bool Write(Json::Value record, bool terminal = false) noexcept;
	FILE* stream = nullptr;
	std::size_t byteLimit = 0;
	std::size_t bytes = 3; // Existing FileSink emits a UTF-8 BOM.
	std::uint64_t sequence = 0;
	bool failed = false;
	bool ended = false;
};

}
