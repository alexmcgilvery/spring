#pragma once

#include "vma/include/vk_mem_alloc.h"
#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>

struct QueueFamilyIndices {
	int	graphics = -1;
	int	present = -1;
	int	transfer = -1;
	int	compute = -1;
};

// Core vulkan reworked components

struct VulkanCore {
	VkInstance					instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT	debugMessenger = VK_NULL_HANDLE;
	VkAllocationCallbacks*		allocatorCallbacks = nullptr;
};

struct VulkanDevice {
	VkPhysicalDevice			physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties	physicalDeviceProperties;
	VkPhysicalDeviceFeatures	physicalDeviceFeatures;

	VkDevice					logicalDevice = VK_NULL_HANDLE;
	VkPipelineCache				pipelineCache = VK_NULL_HANDLE;
	VkDescriptorPool			descriptorPool = VK_NULL_HANDLE;

	QueueFamilyIndices queueFamilyIndices = {};
	VkQueue						graphicsQueue = VK_NULL_HANDLE;
	VkQueue						presentQueue = VK_NULL_HANDLE;
	VkQueue						transferQueue = VK_NULL_HANDLE;
	VkQueue						computeQueue = VK_NULL_HANDLE;

	VmaAllocator				allocator;
};

struct VulkanTaskControl {
	VkCommandPool				commandPool = VK_NULL_HANDLE;
	VkCommandBuffer				primaryCommandBuffer = VK_NULL_HANDLE;

	VkSemaphore					taskCompleteSemaphore = VK_NULL_HANDLE;
};

struct VulkanBuffer {
	VkBuffer					buffer = VK_NULL_HANDLE;
};

struct VulkanMemoryData {
	VmaMemoryUsage				memoryUsage;
	VmaAllocation				allocation;
	VkDeviceSize				usedMemorySize = 0;
};

struct VulkanWindow {
	SDL_Window*					window = nullptr;
	VkSurfaceKHR				surface = VK_NULL_HANDLE;
	VkSwapchainKHR				swapchain = VK_NULL_HANDLE;

	VkFormat					swapchainImageFormat;
	VkExtent2D					swapchainImageExtent;

	uint32_t					swapchainImageCount = 0;
	uint32_t					currentFrameIndex = 0;
};

struct VulkanImage {
	VkImage						image = VK_NULL_HANDLE;
	VkImageView					imageView = VK_NULL_HANDLE;
	VkFramebuffer				framebuffer = VK_NULL_HANDLE;

	VkExtent2D					imageExtent = {};
};

struct VulkanSwapchainImageStatus {
	uint32_t acquiredImageIndex;

	VkSemaphore					imageAvailableSemaphore = VK_NULL_HANDLE;
};

struct VulkanRenderPass {
	VkRenderPass				renderPass = VK_NULL_HANDLE;
	VkClearValue 				clearValue;
};

struct VulkanGraphicsPipeline {
	VkPipeline					pipeline = VK_NULL_HANDLE;
	VkPipelineLayout			pipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout		descriptorSetLayout = VK_NULL_HANDLE;
};

struct VulkanGraphicsSettings {
	VkPresentModeKHR 			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
};
