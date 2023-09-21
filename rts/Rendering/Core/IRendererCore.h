#pragma once

#include <SDL2/SDL.h>

struct SDL_Window;

class IRendererCore
{
public:
	virtual void PreWindowInitialize() = 0;
	virtual void PostWindowInitialize() = 0;
	virtual bool CreateWindow(const char* title) = 0;
	virtual SDL_Window* CreateSDLWindow(const char* title) = 0;
	virtual void SetRendererStartState() = 0;
	virtual void DestroyWindow() = 0;


	virtual void UpdateWindow() = 0;
	virtual void UpdateViewport() = 0;
	virtual void PresentFrame(bool allowSwapBuffers, bool clearErrors) = 0; // Present Frame
	virtual void SetTimeStamp(uint32_t queryIdx) = 0;
	virtual uint64_t CalculateFrameTimeDelta(uint32_t queryIdx0, uint32_t queryIdx1) const = 0;

	virtual bool UniformDataSupported() const = 0;
	virtual bool ModelUniformDataSuppored() const = 0;
	virtual void ToggleMultisampling() const = 0;

	virtual bool GetDebugStatus() = 0;
	virtual void SetDebugStatus(bool enable) = 0;
	virtual bool GetDebugErrors() = 0;
	virtual void SetDebugErrors(bool enable) = 0;
	virtual bool ToggleDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const = 0;

	virtual void AquireThreadContext() = 0;
	virtual void ReleaseThreadContext() = 0;
};