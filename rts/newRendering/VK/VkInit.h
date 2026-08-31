#if 0
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

	const auto& GetInstanceExtensions() const { return instanceExtensionProperties; }
	const auto& GetPhysDeviceExtensions() const { return physicalDeviceExtensionProperties; }
	const auto& GetPhysDevices() const { return physicalDevices; }
	const auto& GetBestPhysDevice() const { return physicalDevice; }

	const auto& GetLogicalDevice() const { return logicalDevice; }

	std::string GetVulkanAPI() const;
	static std::string GetVulkanAPI(uint32_t apiVersion);

	static VkDeviceSize GetVRAMSize(const vk::PhysicalDevice& physicalDevice);
	static bool HasExtension(const std::vector<vk::ExtensionProperties>& extensions, const char* extensionName);

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
	std::vector<vk::ExtensionProperties> instanceExtensionProperties;
	std::vector<vk::ExtensionProperties> physicalDeviceExtensionProperties;

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
	void InitializeVulkanForSDL(SDL_Window* sdlWindow);
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

#endif
