/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace runtime {

enum class TraceKind : std::uint8_t {
	PhaseBegin, PhaseEnd, GuardAcquire, GuardRelease, Controller,
	SessionBegin, SessionEnd, Message, Frame, Publication, Count
};

enum class Phase : std::uint8_t {
	Host, Input, Save, Reload, Update, Presentation, Draw, Capture, Swap,
	Simulation, Count
};

// Fixed-size hot records: no strings, allocation, serialization, or live objects.
struct TraceRecord {
	TraceKind kind = TraceKind::PhaseBegin;
	Phase phase = Phase::Host;
	std::uint64_t time = 0;
	std::array<std::uint64_t, 8> values = {};
};

class BoundedTraceBuffer {
public:
	enum class Mode { Off, Summary, Detailed };

	// Allocation happens once at startup. A failed allocation leaves capture off.
	bool Configure(Mode newMode, std::size_t byteLimit) noexcept;
	void Append(const TraceRecord& record) noexcept;
	std::span<const TraceRecord> Records() const noexcept { return {records.get(), size}; }
	void Clear() noexcept { size = 0; }
	void Invalidate() noexcept { valid = false; }
	bool Valid() const noexcept { return valid; }
	bool Enabled() const noexcept { return mode != Mode::Off; }
	Mode GetMode() const noexcept { return mode; }
	std::uint64_t Dropped() const noexcept { return dropped; }
	std::size_t RetainedBytes() const noexcept { return capacity * sizeof(TraceRecord); }
	const auto& Counts() const noexcept { return counts; }

private:
	Mode mode = Mode::Off;
	bool valid = true;
	std::size_t size = 0;
	std::size_t capacity = 0;
	std::uint64_t dropped = 0;
	std::array<std::uint64_t, static_cast<std::size_t>(TraceKind::Count)> counts = {};
	std::unique_ptr<TraceRecord[]> records;
};

}
