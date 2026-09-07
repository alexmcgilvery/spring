/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <atomic>
#include <cstddef>
#include <limits>
#include <memory_resource>
#include <new>

namespace runtime {

// Share one resource across sessions, catalogs, registries and retained leases.
// Reader-thread destruction is safe; allocation never waits for a reader.
class PublicationMemoryBudget final: public std::pmr::memory_resource {
public:
	explicit PublicationMemoryBudget(std::size_t limit);
	std::size_t RetainedBytes() const noexcept;
	std::size_t PeakBytes() const noexcept;
	std::size_t Limit() const noexcept;

private:
	static std::size_t Charge(std::size_t bytes, std::size_t alignment);
	void* do_allocate(std::size_t bytes, std::size_t alignment) override;
	void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override;
	bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;
	const std::size_t limit;
	std::atomic<std::size_t> retained = 0;
	std::atomic<std::size_t> peak = 0;
};

}
