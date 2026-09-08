/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>

namespace runtime {

/**
 * Schema outline only: no producer constructs this as a usable world snapshot.
 * Expected owned families: authoritative unit/feature/projectile state, articulated
 * pose, visibility knowledge, catalog leases and observation bounds. Identity,
 * revision and completed tick are carried by the manager's publication envelope.
 * Extraction, coverage validation and production memory policy remain unimplemented.
 */
struct PublishedSimFrame {
public:
	// Detailed data fields follow source annotation and non-mutating read validation.
};

using PublishedFrameLease = std::shared_ptr<const PublishedSimFrame>;

} // namespace runtime
