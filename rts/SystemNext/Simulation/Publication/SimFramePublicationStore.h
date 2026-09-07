/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <utility>

#include "PublicationMemoryBudget.h"
#include "PublishedSimFrame.h"

namespace runtime {

// allocate_shared retains this allocator in its control block, so the memory
// resource outlives both the object and the last weak/strong reference.
template<typename T>
class PublicationAllocator {
public:
	using value_type = T;
	explicit PublicationAllocator(std::shared_ptr<PublicationMemoryBudget> budget): budget(std::move(budget)) {}
	template<typename U> PublicationAllocator(const PublicationAllocator<U>& other): budget(other.budget) {}
	T* allocate(std::size_t count) {
		if (count > std::numeric_limits<std::size_t>::max() / sizeof(T))
			throw std::bad_alloc();
		return static_cast<T*>(budget->allocate(count * sizeof(T), alignof(T)));
	}
	void deallocate(T* pointer, std::size_t count) { budget->deallocate(pointer, count * sizeof(T), alignof(T)); }
	template<typename U> bool operator==(const PublicationAllocator<U>& other) const { return budget == other.budget; }
	template<typename> friend class PublicationAllocator;
private:
	std::shared_ptr<PublicationMemoryBudget> budget;
};

enum class ObservationFailure { None, StorageLimit, CounterOverflow, ExtractionFailure, InvalidContract, EventOverflow, Detached };
enum class PublishResult { Published, Unchanged, Skipped, Invalid };

class SimFramePublicationStore {
public:
	explicit SimFramePublicationStore(std::shared_ptr<PublicationMemoryBudget> memory): memory(std::move(memory)) {}
	bool Restart() noexcept;
	void Detach() noexcept;
	void Invalidate(ObservationFailure reason) noexcept;
	bool Valid() const noexcept { return failure == ObservationFailure::None; }
	ObservationFailure Failure() const noexcept { return failure; }
	std::uint64_t Epoch() const noexcept { return epoch; }
	std::uint64_t SkippedStates() const noexcept { return skipped; }
	SimFrameLease Latest() const noexcept { return latest; }
	bool Accepts(const SimFrameLease& lease) const noexcept { return Valid() && lease && lease->epoch == epoch; }
	const auto& Memory() const noexcept { return memory; }

	PublishResult TryPublish(std::int32_t tick, const std::function<void(PublishedSimFrame&)>& extract) noexcept;

	CatalogLease CreateCatalog(std::uint64_t catalogRevision, const std::function<void(PublishedCatalog&)>& fill) noexcept;

private:
	static void Clear(PublishedSimFrame& frame);
	std::shared_ptr<PublicationMemoryBudget> memory;
	std::array<std::shared_ptr<PublishedSimFrame>, 4> slots;
	SimFrameLease latest;
	std::uint64_t epoch = 0;
	std::uint64_t revision = 0;
	std::uint64_t skipped = 0;
	ObservationFailure failure = ObservationFailure::Detached;
};

}
