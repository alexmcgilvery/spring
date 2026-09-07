/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include <bit>
#include <stdexcept>
#include <thread>
#include <type_traits>

#include "SystemNext/Simulation/Publication/SimFramePublicationStore.h"

using namespace runtime;

static_assert(std::is_const_v<SimFrameLease::element_type>);
static_assert(std::is_const_v<CatalogLease::element_type>);

TEST_CASE("Publication distinguishes ticks, no-tick revisions and exact numeric payloads")
{
	auto memory = std::make_shared<PublicationMemoryBudget>(1024 * 1024);
	SimFramePublicationStore store(memory);
	REQUIRE(store.Restart());
	const auto extract = [](PublishedSimFrame& frame) {
		PublishedUnit unit;
		unit.key = {EntityKind::Unit, 7, 1, frame.epoch};
		unit.health = std::bit_cast<float>(std::uint32_t(0x7fc00001));
		frame.units.push_back(unit);
	};
	REQUIRE(store.TryPublish(0, extract) == PublishResult::Published);
	CHECK(store.Latest()->revision == 1);
	CHECK(store.TryPublish(0, extract) == PublishResult::Unchanged);
	CHECK(store.Latest()->revision == 1);
	CHECK(store.TryPublish(1, extract) == PublishResult::Published);
	CHECK(store.TryPublish(2, extract) == PublishResult::Published);
	CHECK(store.Latest()->revision == 3);
	CHECK(store.TryPublish(2, [&](PublishedSimFrame& frame) { extract(frame); frame.paused = true; }) == PublishResult::Published);
	CHECK(store.Latest()->revision == 4);
	CHECK(store.TryPublish(2, [&](PublishedSimFrame& frame) { extract(frame); frame.paused = true; frame.endEvent = 1; }) == PublishResult::Published);
	CHECK(store.Latest()->revision == 5);
	CHECK(SimFloat(0.0f) != SimFloat(-0.0f));
}

TEST_CASE("Four frame slots preserve pinned data and skip without overwriting")
{
	auto memory = std::make_shared<PublicationMemoryBudget>(1024 * 1024);
	SimFramePublicationStore store(memory);
	REQUIRE(store.Restart());
	std::array<SimFrameLease, 4> readers;
	for (int tick = 0; tick < 4; ++tick) {
		REQUIRE(store.TryPublish(tick, [](auto&) {}) == PublishResult::Published);
		readers[tick] = store.Latest();
	}
	CHECK(store.TryPublish(4, [](auto&) { FAIL("Extraction must not run without storage"); }) == PublishResult::Skipped);
	CHECK(store.SkippedStates() == 1);
	CHECK(store.Valid());
	for (int tick = 0; tick < 4; ++tick)
		CHECK(readers[tick]->tick == tick);
	const auto retained = memory->RetainedBytes();
	store.Detach();
	CHECK_FALSE(store.Accepts(readers[0]));
	REQUIRE(store.Restart());
	CHECK(store.TryPublish(0, [](auto&) {}) == PublishResult::Skipped);
	CHECK(memory->RetainedBytes() == retained);
	readers[0].reset();
	REQUIRE(store.TryPublish(0, [](auto&) {}) == PublishResult::Published);
	CHECK(store.Latest()->epoch == 2);
	CHECK(readers[1]->epoch == 1);
	CHECK_FALSE(store.Accepts(readers[1]));
}

TEST_CASE("Frame and catalog leases survive producer and resource-owner teardown")
{
	SimFrameLease reader;
	std::weak_ptr<PublicationMemoryBudget> weakBudget;
	{
		auto memory = std::make_shared<PublicationMemoryBudget>(1024 * 1024);
		weakBudget = memory;
		SimFramePublicationStore store(memory);
		REQUIRE(store.Restart());
		auto catalog = store.CreateCatalog(1, [&](PublishedCatalog& catalog) {
			catalog.models.emplace_back(memory.get());
			catalog.models.back().id = 8;
			catalog.models.back().name = "owned model identity exceeding small-string storage";
		});
		REQUIRE(catalog);
		REQUIRE(store.TryPublish(10, [&](auto& frame) { frame.catalog = catalog; }) == PublishResult::Published);
		reader = store.Latest();
		auto nextCatalog = store.CreateCatalog(2, [](PublishedCatalog&) {});
		REQUIRE(store.TryPublish(11, [&](auto& frame) { frame.catalog = nextCatalog; }) == PublishResult::Published);
		CHECK(reader->catalog->revision == 1);
		CHECK(store.Latest()->catalog->revision == 2);
	}
	CHECK_FALSE(weakBudget.expired());
	CHECK(reader->tick == 10);
	CHECK(reader->catalog->models[0].name == "owned model identity exceeding small-string storage");
	std::thread release([lease = std::move(reader)]() mutable { lease.reset(); });
	release.join();
	CHECK(weakBudget.expired());
}

TEST_CASE("Publication memory and extraction failures invalidate until explicit restart")
{
	auto memory = std::make_shared<PublicationMemoryBudget>(4096);
	SimFramePublicationStore store(memory);
	REQUIRE(store.Restart());
	CHECK(store.TryPublish(0, [](auto& frame) { frame.units.resize(10000); }) == PublishResult::Invalid);
	CHECK(store.Failure() == ObservationFailure::StorageLimit);
	CHECK(memory->RetainedBytes() <= memory->Limit());
	CHECK(store.TryPublish(1, [](auto&) { FAIL("Invalid observation cannot extract"); }) == PublishResult::Invalid);
	REQUIRE(store.Restart());
	CHECK(store.TryPublish(0, [](auto&) { throw std::runtime_error("observer failure"); }) == PublishResult::Invalid);
	CHECK(store.Failure() == ObservationFailure::ExtractionFailure);
	REQUIRE(store.Restart());
	CHECK(store.TryPublish(0, [](auto&) {}) == PublishResult::Published);
}
