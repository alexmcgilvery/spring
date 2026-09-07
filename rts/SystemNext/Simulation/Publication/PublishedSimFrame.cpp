/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PublishedSimFrame.h"

namespace runtime {

SimFloat::SimFloat(float value): value(value)
{}

CatalogDefinition::CatalogDefinition(std::pmr::memory_resource* memory): name(memory)
{}

CatalogModel::CatalogModel(std::pmr::memory_resource* memory): name(memory)
{}

PublishedCatalog::PublishedCatalog(std::pmr::memory_resource* memory): models(memory), definitions(memory), pieces(memory)
{}

PublishedSimFrame::PublishedSimFrame(std::pmr::memory_resource* memory): units(memory), features(memory), projectiles(memory), poses(memory), ghosts(memory), radarErrorSizes(memory)
{}

bool SimFloat::operator==(const SimFloat& other) const noexcept
{
	return std::bit_cast<std::uint32_t>(value) == std::bit_cast<std::uint32_t>(other.value);
}

bool PublishedSimFrame::SameCoveredState(const PublishedSimFrame& other) const
{
	return epoch == other.epoch && tick == other.tick && endEvent == other.endEvent && paused == other.paused
		&& speed == other.speed && wantedSpeed == other.wantedSpeed && catalog == other.catalog
		&& units == other.units && features == other.features && projectiles == other.projectiles
		&& poses == other.poses && ghosts == other.ghosts && radarErrorSizes == other.radarErrorSizes;
}

}
