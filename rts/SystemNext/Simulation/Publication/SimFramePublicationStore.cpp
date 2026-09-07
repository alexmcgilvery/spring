/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimFramePublicationStore.h"

namespace runtime {

bool SimFramePublicationStore::Restart() noexcept
{
	if (epoch == std::numeric_limits<std::uint64_t>::max()) {
		Invalidate(ObservationFailure::CounterOverflow);
		return false;
	}
	++epoch;
	revision = 0;
	latest.reset();
	failure = ObservationFailure::None;
	return true;
}

void SimFramePublicationStore::Detach() noexcept
{
	latest.reset();
	failure = ObservationFailure::Detached;
}

void SimFramePublicationStore::Invalidate(ObservationFailure reason) noexcept
{
	if (Valid())
		failure = reason;
}

void SimFramePublicationStore::Clear(PublishedSimFrame& frame)
{
	frame.epoch = frame.revision = frame.firstEvent = frame.endEvent = 0;
	frame.tick = -1;
	frame.paused = false;
	frame.speed = frame.wantedSpeed = {};
	frame.catalog.reset();
	frame.units.clear();
	frame.features.clear();
	frame.projectiles.clear();
	frame.poses.clear();
	frame.ghosts.clear();
	frame.radarErrorSizes.clear();
}


PublishResult SimFramePublicationStore::TryPublish(std::int32_t tick, const std::function<void(PublishedSimFrame&)>& extract) noexcept
{
	if (!Valid())
		return PublishResult::Invalid;
	// The comparison reader pins the previous frame until extraction ends.
	// Never overwrite its data while checking a no-tick observation revision.
	std::shared_ptr<PublishedSimFrame>* target = nullptr;
	for (auto& slot: slots) {
		if (!slot || slot.use_count() == 1) {
			target = &slot;
			break;
		}
	}
	if (target == nullptr) {
		++skipped;
		return PublishResult::Skipped;
	}
	try {
		if (!*target)
			*target = std::allocate_shared<PublishedSimFrame>(PublicationAllocator<PublishedSimFrame>(memory), memory.get());
		auto& frame = **target;
		Clear(frame);
		frame.epoch = epoch;
		frame.tick = tick;
		extract(frame);
		if (frame.epoch != epoch || frame.tick != tick || frame.firstEvent > frame.endEvent) {
			Invalidate(ObservationFailure::InvalidContract);
			return PublishResult::Invalid;
		}
		if (latest && frame.SameCoveredState(*latest))
			return PublishResult::Unchanged;
		if (revision == std::numeric_limits<std::uint64_t>::max()) {
			Invalidate(ObservationFailure::CounterOverflow);
			return PublishResult::Invalid;
		}
		frame.revision = ++revision;
		latest = *target;
		return PublishResult::Published;
	} catch (const std::bad_alloc&) {
		Invalidate(ObservationFailure::StorageLimit);
	} catch (...) {
		Invalidate(ObservationFailure::ExtractionFailure);
	}
	return PublishResult::Invalid;
}

CatalogLease SimFramePublicationStore::CreateCatalog(std::uint64_t catalogRevision, const std::function<void(PublishedCatalog&)>& fill) noexcept
{
	if (!Valid())
		return {};
	try {
		auto catalog = std::allocate_shared<PublishedCatalog>(PublicationAllocator<PublishedCatalog>(memory), memory.get());
		catalog->revision = catalogRevision;
		fill(*catalog);
		return catalog;
	} catch (const std::bad_alloc&) {
		Invalidate(ObservationFailure::StorageLimit);
	} catch (...) {
		Invalidate(ObservationFailure::ExtractionFailure);
	}
	return {};
}

}
