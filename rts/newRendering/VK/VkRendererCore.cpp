
#include "VkRendererCore.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"


#include <SDL.h>
#include <SDL2/SDL_vulkan.h>

#if defined(HAS_VULKAN) && !defined(HEADLESS)


void CVkRendererCore::RendererPreWindowInit()
{

}

void CVkRendererCore::RendererPostWindowInit()
{

}

void CVkRendererCore::RendererSetStartState()
{
	LOG("[GR::%s]", __func__);

	//vkCoreObject.InitializeVulkanForSDL(sdlWindow);
	//vkCoreObject.InitializeVulkanSwapchainForSDL();
}

SDL_Window* CVkRendererCore::RendererCreateSDLWindow(const char* title)
{
	SDL_Window* newWindow = nullptr;

	const char* frmts[2] = {
		"[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer for main window",
		"[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\") for main window",
	};

	bool borderless_ = configHandler->GetBool("WindowBorderless");
	bool fullScreen_ = configHandler->GetBool("Fullscreen");
	int winPosX_ = configHandler->GetInt("WindowPosX");
	int winPosY_ = configHandler->GetInt("WindowPosY");
	int2 newRes = GetCfgWinRes();

	uint32_t 	sdlFlags  = (borderless_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) * fullScreen_;
				sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless_);
				sdlFlags |= (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_Vulkan_LoadLibrary(nullptr);
	
	if ((newWindow = SDL_CreateWindow(title, winPosX_, winPosY_, newRes.x, newRes.y, sdlFlags)) == nullptr ){
		LOG_L(L_WARNING, frmts[0], __func__ ,SDL_GetError());
	};
}

#endif
