/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlowResult.h"

namespace runtime {
IncompleteFlow::IncompleteFlow(std::string_view id, std::string_view reason):
	std::runtime_error(std::string(reason)), id(id) {}
BlockedFlow IncompleteFlow::Failure() const { return {id, what()}; }
RenderResult RenderResult::Ready() { return {RenderState::Ready, {}}; }
RenderResult RenderResult::Skipped() { return {RenderState::Skipped, {}}; }
RenderResult RenderResult::Blocked(std::string_view id, std::string_view reason)
{
	return {RenderState::Blocked, {std::string(id), std::string(reason)}};
}
bool RenderResult::AllowPresent() const { return state == RenderState::Ready; }
}
