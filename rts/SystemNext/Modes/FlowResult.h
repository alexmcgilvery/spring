/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace runtime {
struct BlockedFlow {
	std::string id;
	std::string reason;
};

/** Carry missing required work through void callbacks and background tasks. */
class IncompleteFlow final : public std::runtime_error {
public:
	IncompleteFlow(std::string_view id, std::string_view reason);
	BlockedFlow Failure() const;
private:
	std::string id;
};

enum class RenderState { Ready, Skipped, Blocked };
struct RenderResult {
	RenderState state;
	BlockedFlow blocked;
	static RenderResult Ready();
	static RenderResult Skipped();
	static RenderResult Blocked(std::string_view id, std::string_view reason);
	bool AllowPresent() const;
};
}
