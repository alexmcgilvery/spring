/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Graphics.h"

namespace runtime {

RenderCommands::~RenderCommands() = default;
RenderResource::~RenderResource() = default;
Graphics::~Graphics() = default;

// TODO(SystemNext): Add concrete graphics implementations behind this interface.
// A backend must select immutable output-target facts, execute owning render
// commands, retain resources until completion, and present only the exact target
// generation recorded by RenderedOutput. This translation unit currently defines
// type lifetimes only; it creates no device, target, renderer, or presenter.

} // namespace runtime
