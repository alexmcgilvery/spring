#pragma once

#include "Rendering/GlobalRendering.h"
#include "newRendering/NewGlobalRendering.h"

#if defined(HAS_VULKAN) && !defined(HEADLESS)

class CVkRendererCore : CGlobalRendering
{
public:
	void RendererPreWindowInit();
	void RendererPostWindowInit();
	SDL_Window* RendererCreateSDLWindow(const char* title);
	void RendererSetStartState();
};

#endif