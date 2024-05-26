
#include "Rendering/GlobalRendering.h"
#include "newRendering/NewGlobalRendering.h"

#if defined(HAS_VULKAN) && !defined(HEADLESS)

class CVkRendererCore : CGlobalRendering
{
public:
	void RendererPreWindowInit() override;
	void RendererPostWindowInit() override;
	bool RendererCreateWindow(const char* title) override;
	SDL_Window* RendererCreateSDLWindow(const char* title)override;
	void RendererSetStartState() override;
	void RendererDestroyWindow() override;
	void RendererUpdateWindow() override;
	void RendererPresentFrame(bool allowSwapBuffers, bool clearErrors) override;

public:
	void UpdateViewport() override;
	void SetTimeStamp(uint32_t queryIdx) const override;
	uint64_t CalculateFrameTimeDelta(uint32_t queryIdx0, uint32_t queryIdx1) const override;

	bool ToggleDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const override;

	// Only one thread can be bound to OpenGL context
	void AquireThreadContext() override;
	void ReleaseThreadContext() override;

public:
	static void InitStatic(){} //todo
	static void KillStatic(){} //todo
};

#endif