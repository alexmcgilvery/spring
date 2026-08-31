
#include "VkObjects.h"
#include "newRendering/GlobalRendering.h"

class CVkRendererCore : CGlobalRendering
{
public:
	CVkRendererCore();
	~CVkRendererCore();

	void RendererPreWindowInit() override;
	void RendererPostWindowInit() override;
	bool RendererCreateWindow(const char* title) override;
	SDL_Window* RendererCreateSDLWindow(const char* title) override;
	void RendererSetStartState() override;
	void RendererDestroyWindow() override;
	void RendererUpdateWindow() override;
	void RendererPresentFrame(bool allowSwapBuffers, bool clearErrors) override;

public:
	void UpdateViewport() override;
	void SetTimeStamp(uint32_t queryIdx) const override;
	uint64_t CalculateFrameTimeDelta(uint32_t queryIdx0, uint32_t queryIdx1) const override;

	void ToggleMultisampling() const;
	bool ToggleDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const override;

	void AquireThreadContext() override;
	void ReleaseThreadContext() override;

public:
	static void InitStatic();
	static void KillStatic();

private:
	// Initializers
	bool InitializeVulkanForSDL(SDL_Window* window);
	void InitializeVulkanSwapchainForSDL();

	// Frame Update
	void PrepareSwapchainFrame();
	void PresentSwapchainFrame();
	void PrepareRenderTarget();
	void FinalizeRenderTarget();

	// Cleanup
	void TerminateVulkanImages();
	void TerminateVulkanSwapchainImages();
	void TerminateVulkanGraphicsPipeline();
	void TerminateVulkanRenderPass();
	void TerminateVulkanSwapchainForSDL();
	void TerminateVulkanSwapchainTaskControls();
	void TerminateVulkanDevices();
	void TerminateVulkanCore();

private:
	bool vkInitialized = false;

	VulkanCore				vkCore;
	VulkanDevice			vkDevice;
	VulkanWindow			vkWindow;
	VulkanRenderPass		vkRenderPass;
	VulkanGraphicsPipeline	vkGraphicsPipeline;
	VulkanGraphicsSettings	vkGraphicsSettings;

	std::vector<VulkanTaskControl>				vkSwapchainCommandData;
	std::vector<VulkanSwapchainImageStatus>		vkSwapchainImageStatuses;
	std::vector<VulkanImage>					vkSwapchainImages;

	std::vector<VulkanImage>					vkImages; // Non swapchain images
};
