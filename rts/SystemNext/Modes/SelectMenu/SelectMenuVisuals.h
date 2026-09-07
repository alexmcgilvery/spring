/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {
class ISelectMenuServices;

/**
 * Compose selection-menu visuals without taking ownership of the menu object.
 * The caller has already resolved visual eligibility and acquired the loading
 * guard. A successful frame permits the caller's ordinary present; failures
 * propagate to the existing outer handler. This abstract companion is inactive
 * until a reviewed backing-service implementation supplies its operations.
 */
class SelectMenuVisuals {
public:
	virtual ~SelectMenuVisuals();
	bool PrepareAndRender();

protected:
	virtual ISelectMenuServices& GetServices() = 0;
};
}
