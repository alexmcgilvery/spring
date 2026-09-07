/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <span>

#include "SimulationNotifications.h"
#include "SystemNext/Simulation/Publication/SimFramePublicationStore.h"

namespace runtime {

struct SimulationEvent {
	std::uint64_t sequence;
	std::int32_t tick;
	NotificationPayload payload;
};

struct SimulationEventBatch {
	std::uint64_t epoch = 0;
	std::uint64_t first = 0;
	std::pmr::vector<SimulationEvent> events;
	explicit SimulationEventBatch(std::pmr::memory_resource* memory): events(memory) {}
};

class SimulationEventCursor {
public:
	SimulationEventCursor(std::uint64_t epoch, std::uint64_t next): epoch(epoch), next(next), delivered(next) {}
	std::uint64_t Next() const noexcept { return next.load(); }
private:
	friend class SimulationEventJournal;
	const std::uint64_t epoch;
	std::atomic<std::uint64_t> next;
	std::uint64_t delivered;
};

class SimulationEventLease {
public:
	std::span<const SimulationEvent> Events() const noexcept { return batch ? std::span<const SimulationEvent>(batch->events).subspan(offset) : std::span<const SimulationEvent>(); }
	std::uint64_t Epoch() const noexcept { return batch ? batch->epoch : 0; }
	explicit operator bool() const noexcept { return batch != nullptr; }
private:
	friend class SimulationEventJournal;
	std::shared_ptr<const SimulationEventBatch> batch;
	std::size_t offset = 0;
};

class SimulationEventJournal {
public:
	explicit SimulationEventJournal(std::shared_ptr<PublicationMemoryBudget> memory): memory(std::move(memory)), batches(this->memory.get()), readers(this->memory.get()) {}
	void Restart(std::uint64_t newEpoch) noexcept;
	void Detach() noexcept { valid = false; }
	bool Valid() const noexcept { return valid; }
	std::uint64_t End() const noexcept { return next; }
	std::uint64_t FailureBoundary() const noexcept { return next; }
	std::shared_ptr<SimulationEventCursor> Subscribe() noexcept;
	bool Seal() noexcept;
	SimulationEventLease Read(const std::shared_ptr<SimulationEventCursor>& cursor) noexcept;
	bool Acknowledge(const std::shared_ptr<SimulationEventCursor>& cursor, std::uint64_t end) noexcept;
	std::uint64_t ReaderLag() const noexcept;
	const auto& Memory() const noexcept { return memory; }

	bool Append(std::int32_t tick, const std::function<NotificationPayload()>& copyPayload) noexcept;

private:
	void Collect();
	std::shared_ptr<PublicationMemoryBudget> memory;
	std::pmr::deque<std::shared_ptr<const SimulationEventBatch>> batches;
	std::pmr::vector<std::weak_ptr<SimulationEventCursor>> readers;
	std::shared_ptr<SimulationEventBatch> current;
	std::uint64_t epoch = 0;
	std::uint64_t next = 0;
	bool valid = false;
};

}
