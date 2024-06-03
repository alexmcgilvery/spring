#include "VkObjects.h"

#include <vector>
#include <memory>

/**

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

class VkCoreObjects {
public:
	VkCoreObjects();
	~VkCoreObjects();

	const auto& GetInstanceExtensions() const { return instExtensionProperties; }
	const auto& GetPhysDeviceExtensions() const { return pdevExtensionProperties; }
	const auto& GetPhysDevices() const { return physicalDevices; }
	const auto& GetBestPhysDevice() const { return physicalDevice; }

	const auto& GetLogicalDevice() const { return logicalDevice; }

	std::string GetVulkanAPI() const;
	static std::string GetVulkanAPI(uint32_t apiVersion);

	static VkDeviceSize GetVRAMSize(const vk::PhysicalDevice& phd);
	static bool HasExtension(const std::vector<vk::ExtensionProperties>& es, const char* e);

	static VkCoreObjects& GetInstance() {
		if (!vkCoreObjects) {
			vkCoreObjects = std::make_unique<VkCoreObjects>();
		}
		return *vkCoreObjects;
	}
	static void KillInstance() { vkCoreObjects = nullptr; }

	bool IsValid() const { return vkInitialized; }
private:
	static inline std::unique_ptr<VkCoreObjects1> vkCoreObjects = nullptr;

	bool vkInitialized = false;

	vk::Instance instance;
	std::vector<vk::ExtensionProperties> instExtensionProperties;
	std::vector<vk::ExtensionProperties> pdevExtensionProperties;

	std::vector<vk::PhysicalDevice> physicalDevices;
	vk::PhysicalDevice physicalDevice;

	uint32_t apiVersion;

	vk::Device logicalDevice;

	//static constexpr 
};

#endif
*/

class VkCoreObjects {
public:
	VkCoreObjects();
	~VkCoreObjects();

	const VkDeviceSize GetCurrentDeviceVRAMSize();

private:
	// Initializers
	void InitializeVulkanForSDL(SDL_Window* _sdl_window_ptr);
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
	bool vkInitialized_ = false;

	VulkanCore				vkCore_;
	VulkanDevice			vkDevice_;
	VulkanWindow			vkWindow_;
	VulkanRenderPass		vkRenderPass_;
	VulkanGraphicsPipeline	vkGraphicsPipeline_;
	VulkanGraphicalSettings	vkGraphicsSettings_;

	std::vector<VulkanTaskControl>				vkSwapchainCommandData_;
	std::vector<VulkanSwapchainImageStatus>		vkSwapchainImageStatuses_;
	std::vector<VulkanImage>					vkSwapchainImages_;

	std::vector<VulkanImage>					vkImages_; // Non swapchain images
};