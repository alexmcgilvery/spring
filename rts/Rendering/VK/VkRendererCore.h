#pragma once

#include "Rendering/GlobalRendering.h"

#if defined(HAS_VULKAN) && !defined(HEADLESS)

class CVkRendererCore final : IGlobalRendering
{
public:
	void RendererPreWindowInit();
	void RendererPostWindowInit();
	SDL_Window* RendererCreateSDLWindow(const char* title);
	void RendererSetStartState();
};

#endif