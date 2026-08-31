#include "VkObjects.h"

#include <SDL.h>
#include <vulkan/vulkan.h>

#include <vector>

namespace vulkan
{
	void								InitializeVulkanInstance(
											VkInstance&						instance,
											VkAllocationCallbacks*			allocatorCallbacks,
											VkDebugUtilsMessengerEXT&		debugMessenger,
											std::vector<const char*>		instanceExtensions = {},
											std::vector<const char*>		instanceLayers = {}
										);

	void								InitializeVulkanDevice(
											const VkPhysicalDevice&			physicalDevice,
											QueueFamilyIndices&				queueFamilyIndices,
											VkDevice&						logicalDevice,
											const VkSurfaceKHR				surface = VK_NULL_HANDLE,
											std::vector<const char*>		deviceExtensions = {},
											std::vector<const char*>		layers = {}
										);

	void								InitializeVulkanDescriptorPool(
											const VkDevice&					logicalDevice,
											const VkAllocationCallbacks* 	allocatorCallbacks,
											VkDescriptorPool& 				descriptorPool
										);

	// information
	std::vector<VkPhysicalDevice> 		DeterminePhysicalDevices(
											const VkInstance& 				instance,
											std::vector<const char*>		requiredExtensions = {},
											std::vector<const char*>		requiredLayers = {}
										);

	void								TerminateVulkanInstance(
											const VkInstance&				instance,
											const VkAllocationCallbacks*	allocatorCallbacks,
											VkDebugUtilsMessengerEXT&		debugMessenger
										);


	bool								CheckInstanceLayerSupport(std::vector<const char*> requiredLayers);

	namespace data
	{
		VkResult						CreateOrResizeBuffer(
											const VmaAllocator&				vmaAllocator,
											const VkBufferCreateInfo&		bufferCreateInfo,
											const VmaMemoryUsage&			memoryUsage,
											VkBuffer&						buffer,
											VmaAllocation&					allocation
										);

		VkResult						CreateOrResizeImage(
											const VmaAllocator&				vmaAllocator,
											const VkImageCreateInfo&		imageCreateInfo,
											const VmaMemoryUsage&			memoryUsage,
											VkImage&						image,
											VmaAllocation&					allocation
										);
	}

	namespace validation
	{
		static VKAPI_ATTR VkBool32		DebugMessengerCallback(
											VkDebugUtilsMessageSeverityFlagBitsEXT		messageSeverity,
											VkDebugUtilsMessageTypeFlagsEXT				messageType,
											const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
											void*										userData
										);

		void							SetupDebugUtilsMessenger(
											const VkInstance&							instance,
											const VkAllocationCallbacks*				allocatorCallbacks,
											VkDebugUtilsMessengerEXT&					debugMessenger
										);

		VkResult						CreateDebugUtilsMessengerEXT(
											const VkInstance&							instance,
											const VkAllocationCallbacks* 				allocatorCallbacks,
											const VkDebugUtilsMessengerCreateInfoEXT* 	createInfo,
											VkDebugUtilsMessengerEXT& 					debugMessenger
										);

		void							DestroyDebugUtilsMessengerEXT(
											const VkInstance&							instance,
											const VkAllocationCallbacks*				allocatorCallbacks,
											VkDebugUtilsMessengerEXT&					debugMessenger
										);
	}


	std::vector<const char*>			GetRequiredSDLExtensions(SDL_Window* window);

	std::vector<VkImage>				InitializeVulkanSwapchainForSDL(
											const VkPhysicalDevice				physicalDevice,
											const VkDevice						logicalDevice,
											const VkAllocationCallbacks* 		allocatorCallbacks,
											const VkSurfaceKHR&					surface,
											SDL_Window* const					window,
											VkSwapchainKHR&						swapchain,
											VkFormat&							imageFormat,
											VkExtent2D&							imageExtent
										);

	void								InitializeVulkanRenderPass(
											const VkDevice&						device,
											const VkFormat&						format,
											VkRenderPass&						renderPass
										);

	void								InitializeGraphicsPipeline(
											const VkDevice&						device,
											const VkPipelineCache&				pipelineCache,
											const VkAllocationCallbacks*		allocatorCallbacks,
											const VkRenderPass&					renderPass,
											const VkExtent2D					extent,
											VkPipelineLayout&					pipelineLayout,
											VkDescriptorSetLayout&				descriptorSetLayout,
											VkPipeline&							pipeline
										);



	namespace device_support
	{
		struct DeviceSwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR				capabilities;
			std::vector<VkSurfaceFormatKHR>			formats;
			std::vector<VkPresentModeKHR>			presentModes;
		};

		QueueFamilyIndices				DetermineQueueFamilies(
										const VkPhysicalDevice&						physicalDevice,
										const VkSurfaceKHR&							surface = VK_NULL_HANDLE
										);

		bool							CheckDeviceExtensionSupport(
											const VkPhysicalDevice&					physicalDevice,
											std::vector<const char*>				requiredExtensionNames = {}
										);

		DeviceSwapchainSupportDetails	QueryDeviceSwapchainSupport(
											const VkPhysicalDevice&					physicalDevice,
											const VkSurfaceKHR&						surface
										);

		VkSurfaceFormatKHR				ChooseSwapSurfaceFormat(
											const std::vector<VkSurfaceFormatKHR>&	availableFormats
										);

		VkPresentModeKHR				ChooseSwapPresentMode(
											const std::vector<VkPresentModeKHR>		availableModes
										);

		VkExtent2D						ChooseSwapExtent(
											const VkSurfaceCapabilitiesKHR&			capabilities,
											SDL_Window* const						window
										);
	}
}
