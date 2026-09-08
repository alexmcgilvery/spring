/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "PublicationContext.h"

namespace runtime {

/*
 * Game-local publication expectations only. No frame schema, storage pool, lease
 * implementation or extraction adapter exists in this skeleton.
 */
class Publication {
public:
	void Publish(const PublicationContext& context);
};

} // namespace runtime
