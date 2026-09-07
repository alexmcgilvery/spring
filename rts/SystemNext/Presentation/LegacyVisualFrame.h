/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <optional>
#include "SerialVisualFrame.h"
#include "System/Platform/Threading.h"
#include "System/LoadLock.h"

namespace runtime::legacy {
/** Bind serial visual execution to current controller and graphics operations. */
class LegacyVisualFrame final : public SerialVisualFrame {
protected:
	void LockDraw() override;
	void UnlockDraw() noexcept override;
	void Present(bool allowSwap) override;
private:
	bool context = false;
	std::optional<decltype(CLoadLock::GetUniqueLock())> lock;
};
}
