/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PublicationMemoryBudget.h"

namespace runtime {

PublicationMemoryBudget::PublicationMemoryBudget(std::size_t limit): limit(limit)
{}

std::size_t PublicationMemoryBudget::RetainedBytes() const noexcept
{
	return retained.load();
}

std::size_t PublicationMemoryBudget::PeakBytes() const noexcept
{
	return peak.load();
}

std::size_t PublicationMemoryBudget::Limit() const noexcept
{
	return limit;
}

std::size_t PublicationMemoryBudget::Charge(std::size_t bytes, std::size_t alignment)
{
	// Include conservative per-allocation bookkeeping/alignment allowance.
	constexpr std::size_t overhead = 64;
	if (alignment > std::numeric_limits<std::size_t>::max() - overhead || bytes > std::numeric_limits<std::size_t>::max() - overhead - alignment)
		throw std::bad_alloc();
	return bytes + alignment + overhead;
}

void* PublicationMemoryBudget::do_allocate(std::size_t bytes, std::size_t alignment)
{
	const auto charged = Charge(bytes, alignment);
	auto current = retained.load();
	do {
		if (current > limit || charged > limit - current)
			throw std::bad_alloc();
	} while (!retained.compare_exchange_weak(current, current + charged));
	try {
		auto* result = std::pmr::new_delete_resource()->allocate(bytes, alignment);
		auto high = peak.load();
		while (high < current + charged && !peak.compare_exchange_weak(high, current + charged)) {}
		return result;
	} catch (...) {
		retained.fetch_sub(charged);
		throw;
	}
}

void PublicationMemoryBudget::do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment)
{
	std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
	retained.fetch_sub(Charge(bytes, alignment));
}

bool PublicationMemoryBudget::do_is_equal(const std::pmr::memory_resource& other) const noexcept
{
	return this == &other;
}

}
