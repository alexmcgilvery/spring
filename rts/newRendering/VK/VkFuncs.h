#include "VkObjects.h"

#include <SDL.h>
#include <vulkan/vulkan.h>

#include <vector>

namespace Vulkan
{
	void								InitializeVulkanInstance(
											VkInstance&						_instance, 
											VkAllocationCallbacks*			_allocator_ptr, 
											VkDebugUtilsMessengerEXT&		_message_back,
											std::vector<const char*>		_instance_extensions = {}, 
											std::vector<const char*>		_instance_layers = {}
										);

	void								InitializeVulkanDevice(
											const VkPhysicalDevice&			_physical_device, 
											QueueFamilyIndices&				_indices, 
											VkDevice&						_logical_device,  
											const VkSurfaceKHR				_surface = VK_NULL_HANDLE,
											std::vector<const char*>		_device_extensions = {},
											std::vector<const char*>		_layers = {}
										);

	void								InitializeVulkanDescriptorPool(
											const VkDevice&					_logical_device,
											const VkAllocationCallbacks* 	_allocator_ptr, 
											VkDescriptorPool& 				_descriptor_pool
										);

	// information
	std::vector<VkPhysicalDevice> 		DeterminePhysicalDevices(
											const VkInstance& 				_instance,
											std::vector<const char*>		_required_extensions = {},
											std::vector<const char*>		_required_layers = {}
										);

	void								TerminateVulkanInstance(
											const VkInstance&				_instance,
											const VkAllocationCallbacks*	_allocator_ptr,
											VkDebugUtilsMessengerEXT&		_message_back
										);


	bool								CheckInstanceLayerSupport(std::vector<const char*> _required_layers);

	namespace Data
	{
		VkResult						CreateOrResizeBuffer(
											const VmaAllocator&				_vma_allocator,
											const VkBufferCreateInfo&		_buffer_create_info,
											const VmaMemoryUsage&			_memory_usage,
											VkBuffer&						_buffer,
											VmaAllocation&					_memory_allocation
										);

		VkResult						CreateOrResizeImage(
											const VmaAllocator&				_vma_allocator,
											const VkImageCreateInfo&		_image_create_info,
											const VmaMemoryUsage&			_memory_usage,
											VkImage&						_image,
											VmaAllocation&					_memory_allocation
										);
	}
			
	namespace Validation
	{
		static VKAPI_ATTR VkBool32		DebugMessengerCallback(
											VkDebugUtilsMessageSeverityFlagBitsEXT		_message_severity, 
											VkDebugUtilsMessageTypeFlagsEXT				_message_type, 
											const VkDebugUtilsMessengerCallbackDataEXT* _callback_data, 
											void*										_user_data
										);

		void							SetupDebugUtilsMessenger(
											const VkInstance&							_instance, 
											const VkAllocationCallbacks*				_allocator_ptr, 
											VkDebugUtilsMessengerEXT&					_message_back
										);

		VkResult						CreateDebugUtilsMessengerEXT(
											const VkInstance&							_instance, 
											const VkAllocationCallbacks* 				_allocator_ptr, 
											const VkDebugUtilsMessengerCreateInfoEXT* 	_create_info_ptr, 
											VkDebugUtilsMessengerEXT& 					_message_back
										);

		void							DestroyDebugUtilsMessengerEXT(
											const VkInstance&							_instance, 
											const VkAllocationCallbacks*				_allocator_ptr, 
											VkDebugUtilsMessengerEXT&					_message_back
										);
	}	


	std::vector<const char*>			GetRequiredSDLExtensions(SDL_Window* _window_ptr);

	std::vector<VkImage>				InitializeVulkanSwapChainForSDL(
											const VkPhysicalDevice				_physical_device, 
											const VkDevice						_logical_device, 
											const VkAllocationCallbacks* 		_allocator_ptr, 
											const VkSurfaceKHR&					_surface, 
											SDL_Window* const					_window_ptr, 
											VkSwapchainKHR&						_swap_chain, 
											VkFormat&							_image_format, 
											VkExtent2D&							_image_extent
										);

	void								InitializeVulkanRenderPass(
											const VkDevice&						_device, 
											const VkFormat&						_format, 
											VkRenderPass&						_render_pass
										);

	void								InitializeGraphicsPipeline(
											const VkDevice&						_device, 
											const VkPipelineCache&				_pipeline_cache, 
											const VkAllocationCallbacks*		_allocator_ptr, 
											const VkRenderPass&					_render_pass, 
											const VkExtent2D					_extent,  
											VkPipelineLayout&					_pipeline_layout, 
											VkDescriptorSetLayout&				_descriptor_set_layout, 
											VkPipeline&							_pipeline
										);



	namespace DeviceSupport
	{
		struct DeviceSwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR				capabilities;
			std::vector<VkSurfaceFormatKHR>			formats;
			std::vector<VkPresentModeKHR>			present_modes;
		};

		QueueFamilyIndices				DetermineQueueFamilies(
										const VkPhysicalDevice&						_physical_device, 
										const VkSurfaceKHR&							_surface = VK_NULL_HANDLE
										);

		bool							CheckDeviceExtensionSupport(
											const VkPhysicalDevice&					_physical_device, 
											std::vector<const char*>				_required_extensions = {}
										);

		DeviceSwapChainSupportDetails	QueryDeviceSwapChainSupport(
											const VkPhysicalDevice&					_physical_device, 
											const VkSurfaceKHR&						_surface
										);

		VkSurfaceFormatKHR				ChooseSwapSurfaceFormat(
											const std::vector<VkSurfaceFormatKHR>&	_available_formats
										);

		VkPresentModeKHR				ChooseSwapPresentMode(
											const std::vector<VkPresentModeKHR>		_available_modes
										);

		VkExtent2D						ChooseSwapExtent(
											const VkSurfaceCapabilitiesKHR&			_capabilities, 
											SDL_Window* const						_window_ptr
										);
	}
}