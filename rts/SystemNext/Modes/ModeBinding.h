/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
#include "../Globals/InvocationContext.h"
namespace runtime {
class IMode;
/**
 * Borrow a polymorphic mode. nullptr explicitly means inactive, not a sixth mode.
 * The host owns lifetime and must advance generation on every activation,
 * including reuse of the same object/address. Retire backing safely before a
 * replacement is observed; a pointer/generation pair is not a lifetime lease.
 */
struct ActiveModeBinding {
	IMode* mode = nullptr;
	std::uint64_t generation = 0;
	bool operator==(const ActiveModeBinding&) const = default;
};
}
