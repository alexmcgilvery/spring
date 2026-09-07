/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once
#include <cstdint>
namespace runtime {
class Mode;
struct ModeSelection {
	Mode* mode;
	std::uint64_t generation;
	bool operator==(const ModeSelection&) const = default;
};
/** Resolution never grants a lease on the existing backing controller. */
class ModeBinding {
public:
	virtual ~ModeBinding() = default;
	virtual Mode* Resolve() = 0;
	virtual std::uint64_t Generation() const = 0;
	ModeSelection Select();
};
}
