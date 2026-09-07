/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstddef>
#include <cstdint>
#include "SystemNext/Diagnostics/PhaseToken.h"

namespace runtime::legacy {

void InitializeDiagnostics() noexcept;
void DrainDiagnostics() noexcept;
void FinishDiagnostics(bool complete) noexcept;
void BeginSession() noexcept;
void EndSession() noexcept;
void AcceptedMessage(int tick, const std::uint8_t* data, std::size_t length) noexcept;
void CompletedFrame(int tick, std::uint32_t checksum, bool hasChecksum) noexcept;
void ObserveGuard(bool acquire, bool context) noexcept;
void ObserveController(const void* before, const void* after) noexcept;

PhaseToken BeginPhase(Phase phase) noexcept;
void EndPhase(PhaseToken token) noexcept;

class PhaseScope {
public:
	explicit PhaseScope(Phase phase) noexcept;
	~PhaseScope();
	PhaseScope(const PhaseScope&) = delete;
	PhaseScope& operator=(const PhaseScope&) = delete;
private:
	PhaseToken token;
};

}
