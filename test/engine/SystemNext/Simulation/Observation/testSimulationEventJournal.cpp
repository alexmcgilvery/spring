/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include <thread>

#include "SystemNext/Simulation/Observation/SimulationEventJournal.h"

using namespace runtime;

TEST_CASE("Independent event readers retain ordered events across state coalescing")
{
	auto memory = std::make_shared<PublicationMemoryBudget>(1024 * 1024);
	SimulationEventJournal journal(memory);
	journal.Restart(1);
	auto fast = journal.Subscribe();
	auto slow = journal.Subscribe();
	REQUIRE(fast);
	REQUIRE(slow);
	for (int i = 0; i < 100; ++i)
		REQUIRE(journal.Append(i / 3, [i] { return PlayerAddedNotification{i}; }));
	REQUIRE(journal.Seal());
	CHECK(journal.ReaderLag() == 100);
	std::uint64_t expected = 0;
	while (auto lease = journal.Read(fast)) {
		for (const auto& event: lease.Events()) {
			CHECK(event.sequence == expected++);
			CHECK(std::get<PlayerAddedNotification>(event.payload).playerID == int(event.sequence));
		}
		CHECK(journal.Acknowledge(fast, expected));
	}
	CHECK(expected == 100);
	CHECK(journal.ReaderLag() == 100);
	auto pinned = journal.Read(slow);
	REQUIRE(pinned);
	CHECK(pinned.Events()[0].sequence == 0);
	CHECK_FALSE(journal.Acknowledge(slow, 101));
	CHECK(journal.Acknowledge(slow, 64));
	CHECK(journal.ReaderLag() == 36);
	journal.Restart(2);
	CHECK_FALSE(journal.Read(slow));
	CHECK_FALSE(journal.Acknowledge(slow, 64));
	CHECK(pinned.Epoch() == 1);
	CHECK(pinned.Events().size() == 64);
}

TEST_CASE("Owned notification payload and journal lease outlive producer")
{
	SimulationEventLease lease;
	std::weak_ptr<PublicationMemoryBudget> weak;
	{
		auto memory = std::make_shared<PublicationMemoryBudget>(1024 * 1024);
		weak = memory;
		SimulationEventJournal journal(memory);
		journal.Restart(1);
		auto reader = journal.Subscribe();
		std::vector<std::uint8_t> original = {1, 2, 3};
		REQUIRE(journal.Append(0, [&] { return GameOverNotification{std::pmr::vector<std::uint8_t>(original.begin(), original.end(), memory.get())}; }));
		original[0] = 99;
		REQUIRE(journal.Seal());
		lease = journal.Read(reader);
	}
	REQUIRE(lease);
	CHECK_FALSE(weak.expired());
	CHECK(std::get<GameOverNotification>(lease.Events()[0].payload).winningAllyTeams[0] == 1);
	std::thread release([lease = std::move(lease)]() mutable { lease = {}; });
	release.join();
	CHECK(weak.expired());
}

TEST_CASE("A stalled reader invalidates bounded observation without discarding leased data")
{
	auto memory = std::make_shared<PublicationMemoryBudget>(64 * 1024);
	SimulationEventJournal journal(memory);
	journal.Restart(1);
	auto reader = journal.Subscribe();
	REQUIRE(reader);
	REQUIRE(journal.Append(0, [] { return GameStartNotification{}; }));
	REQUIRE(journal.Seal());
	auto lease = journal.Read(reader);
	REQUIRE(lease);
	int accepted = 1;
	while (journal.Append(accepted, [] { return GameStartNotification{}; }))
		++accepted;
	CHECK_FALSE(journal.Valid());
	CHECK(memory->RetainedBytes() <= memory->Limit());
	CHECK_FALSE(journal.Append(0, [] { FAIL("Invalid journal cannot copy more payloads"); return GameStartNotification{}; }));
	CHECK(lease.Events()[0].sequence == 0);
	for (std::uint64_t epoch = 2; epoch < 20; ++epoch) {
		journal.Restart(epoch);
		CHECK(journal.Append(0, [] { return GameStartNotification{}; }));
		CHECK(memory->RetainedBytes() <= memory->Limit());
	}
}
