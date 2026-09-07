/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BoundedTraceBuffer.h"

namespace runtime {

bool BoundedTraceBuffer::Configure(Mode newMode, std::size_t byteLimit) noexcept
{
	mode = Mode::Off;
	records.reset();
	size = capacity = dropped = 0;
	counts.fill(0);
	valid = true;
	if (newMode == Mode::Detailed) {
		capacity = byteLimit / sizeof(TraceRecord);
		if (capacity == 0) {
			valid = false;
			return false;
		}
		try {
			records = std::make_unique<TraceRecord[]>(capacity);
		} catch (...) {
			capacity = 0;
			valid = false;
			return false;
		}
	}
	mode = newMode;
	return true;
}

void BoundedTraceBuffer::Append(const TraceRecord& record) noexcept
{
	if (!Enabled())
		return;
	const auto index = static_cast<std::size_t>(record.kind);
	if (index >= counts.size()) {
		Invalidate();
		return;
	}
	++counts[index];
	if (mode != Mode::Detailed)
		return;
	if (!valid || size == capacity) {
		valid = false;
		++dropped;
		return;
	}
	records[size++] = record;
}

}
