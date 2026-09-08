/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {

/** Application-owned diagnostic buffering and outer-boundary reporting. */
class Diagnostics {
public:
	virtual ~Diagnostics();

	virtual void Report() = 0;
};

} // namespace runtime
