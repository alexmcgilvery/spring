/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimulationEventJournal.h"

#include <algorithm>

namespace runtime {

void SimulationEventJournal::Restart(std::uint64_t newEpoch) noexcept
{
	current.reset();
	batches.clear();
	readers.clear();
	epoch = newEpoch;
	next = 0;
	valid = epoch != 0;
}

std::shared_ptr<SimulationEventCursor> SimulationEventJournal::Subscribe() noexcept
{
	if (!valid)
		return {};
	try {
		Collect();
		auto cursor = std::allocate_shared<SimulationEventCursor>(PublicationAllocator<SimulationEventCursor>(memory), epoch, next);
		readers.push_back(cursor);
		return cursor;
	} catch (...) {
		valid = false;
		return {};
	}
}

bool SimulationEventJournal::Seal() noexcept
{
	if (!valid)
		return false;
	if (!current)
		return true;
	try {
		batches.push_back(current);
		current.reset();
		return true;
	} catch (...) {
		valid = false;
		return false;
	}
}

SimulationEventLease SimulationEventJournal::Read(const std::shared_ptr<SimulationEventCursor>& cursor) noexcept
{
	SimulationEventLease lease;
	if (!valid || !cursor || cursor->epoch != epoch)
		return lease;
	const auto sequence = cursor->Next();
	for (const auto& batch: batches) {
		const auto end = batch->first + batch->events.size();
		if (sequence >= batch->first && sequence < end) {
			lease.batch = batch;
			lease.offset = sequence - batch->first;
			cursor->delivered = end;
			return lease;
		}
	}
	return lease;
}

bool SimulationEventJournal::Acknowledge(const std::shared_ptr<SimulationEventCursor>& cursor, std::uint64_t end) noexcept
{
	if (!valid || !cursor || cursor->epoch != epoch || end < cursor->Next() || end > cursor->delivered)
		return false;
	cursor->next.store(end);
	Collect();
	return true;
}

std::uint64_t SimulationEventJournal::ReaderLag() const noexcept
{
	std::uint64_t oldest = next;
	for (const auto& weak: readers) {
		if (const auto cursor = weak.lock())
			oldest = std::min(oldest, cursor->Next());
	}
	return next - oldest;
}

void SimulationEventJournal::Collect()
{
	readers.erase(std::remove_if(readers.begin(), readers.end(), [](const auto& weak) { return weak.expired(); }), readers.end());
	const auto oldest = next - ReaderLag();
	while (!batches.empty() && batches.front()->first + batches.front()->events.size() <= oldest)
		batches.pop_front();
}


bool SimulationEventJournal::Append(std::int32_t tick, const std::function<NotificationPayload()>& copyPayload) noexcept
{
	if (!valid)
		return false;
	try {
		if (next == std::numeric_limits<std::uint64_t>::max()) {
			valid = false;
			return false;
		}
		if (!current) {
			Collect();
			current = std::allocate_shared<SimulationEventBatch>(PublicationAllocator<SimulationEventBatch>(memory), memory.get());
			current->epoch = epoch;
			current->first = next;
			current->events.reserve(64);
		}
		// Copy before advancing the sequence; failed extraction never becomes
		// a partially accepted event. The observer must not dispatch callbacks.
		current->events.push_back({next, tick, copyPayload()});
		++next;
		return current->events.size() < 64 || Seal();
	} catch (...) {
		valid = false;
		return false;
	}
}

}
