/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include "SystemNext/Simulation/Observation/EntityIdentityRegistry.h"
#include "SystemNext/Simulation/Publication/PublicationMemoryBudget.h"

TEST_CASE("Identity distinguishes semantic death, removal and same-address reuse")
{
	runtime::PublicationMemoryBudget budget(4096);
	runtime::EntityIdentityRegistry identities(&budget);
	identities.Restart(1);
	const auto first = identities.Discover(runtime::EntityKind::Unit, 4, 100);
	identities.SemanticDeath(first);
	CHECK(identities.Dead(first));
	CHECK(identities.Alive(first));
	CHECK(identities.Discover(runtime::EntityKind::Unit, 4, 100) == first);
	identities.Retire(first);
	CHECK_FALSE(identities.Alive(first));
	CHECK(identities.Resolve(runtime::EntityKind::Unit, 4, 100) == first);
	const auto reused = identities.Created(runtime::EntityKind::Unit, 4, 100);
	CHECK(reused.generation == first.generation + 1);
	CHECK_FALSE(identities.Dead(reused));
	identities.Retire(first);
	CHECK(identities.Alive(reused));
	identities.Restart(2);
	const auto reset = identities.Discover(runtime::EntityKind::Unit, 4, 100);
	CHECK(reset.epoch == 2);
	CHECK(reset != reused);
}

TEST_CASE("A discovered projectile retains identity through its delayed creation notification")
{
	runtime::PublicationMemoryBudget budget(4096);
	runtime::EntityIdentityRegistry identities(&budget);
	identities.Restart(7);
	const auto discovered = identities.Discover(runtime::EntityKind::SyncedProjectile, 9, 200, true);
	CHECK(identities.AwaitingCreation(discovered));
	const auto notified = identities.Created(runtime::EntityKind::SyncedProjectile, 9, 200);
	CHECK(discovered == notified);
	CHECK_FALSE(identities.AwaitingCreation(notified));
	CHECK(identities.Resolve(runtime::EntityKind::Unit, 9, 200).kind == runtime::EntityKind::None);
}
