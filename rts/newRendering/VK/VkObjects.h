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

	VkDebugUtilsMessengerEXT	message_back = VK_NULL_HANDLE;
	VkAllocationCallbacks*		allocator_ptr = nullptr;
};

struct VulkanDevice {
	VkPhysicalDevice			physical_device = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties	physical_device_properties;
	VkPhysicalDeviceFeatures	physical_device_features;

	VkDevice					logical_device = VK_NULL_HANDLE;
	VkPipelineCache				pipeline_cache = VK_NULL_HANDLE;
	VkDescriptorPool			descriptor_pool = VK_NULL_HANDLE;

	QueueFamilyIndices			indices = {};
	VkQueue						graphics_queue = VK_NULL_HANDLE;
	VkQueue						present_queue = VK_NULL_HANDLE;
	VkQueue						transfer_queue = VK_NULL_HANDLE;
	VkQueue						compute_queue = VK_NULL_HANDLE;

	VmaAllocator				allocator;
};

struct VulkanTaskControl {
	VkCommandPool				command_pool = VK_NULL_HANDLE;
	VkCommandBuffer				primary_buffer = VK_NULL_HANDLE;

	VkSemaphore					task_complete_semaphore = VK_NULL_HANDLE;
};

struct VulkanBuffer {
	VkBuffer					vkbuffer = VK_NULL_HANDLE;
};

struct VulkanMemoryData {
	VmaMemoryUsage				data_memory_usage;
	VmaAllocation				data_allocation;
	VkDeviceSize				used_memory_size = 0;
};

struct VulkanWindow {
	SDL_Window*					window_ptr = nullptr;
	VkSurfaceKHR				display_surface = VK_NULL_HANDLE;
	VkSwapchainKHR				swapchain = VK_NULL_HANDLE;

	VkFormat					swapchain_image_format;
	VkExtent2D					swapchain_image_extent;

	uint32_t					swapchain_image_count = 0;
	uint32_t					current_frame_index = 0;
};

struct VulkanImage {
	VkImage						image = VK_NULL_HANDLE;
	VkImageView					image_view = VK_NULL_HANDLE;
	VkFramebuffer				framebuffer = VK_NULL_HANDLE;
	
	VkExtent2D					image_extent = {};
};

struct VulkanSwapchainImageStatus {
	uint32_t					frame_index;

	VkSemaphore					image_available_semaphore = VK_NULL_HANDLE;
};

struct VulkanRenderPass {
	VkRenderPass				render_pass = VK_NULL_HANDLE;
	VkClearValue 				clear_value; 
};

struct VulkanGraphicsPipeline {
	VkPipeline					pipeline = VK_NULL_HANDLE;
	VkPipelineLayout			pipeline_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout		descriptor_set_layout = VK_NULL_HANDLE;
};

struct VulkanGraphicalSettings {
	VkPresentModeKHR 			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
};