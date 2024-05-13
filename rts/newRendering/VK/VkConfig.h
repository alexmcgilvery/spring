#pragma once
#if defined(HAS_VULKAN) && !defined(HEADLESS)

#include <vulkan/vulkan.h>
#include <array>

namespace Vulkan::RenderConfig
{
	const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const uint32_t VK_API_VERSION = VK_API_VERSION_1_1;
}

/*
static constexpr std::array requiredDeviceExtensions {
		"VK_KHR_dedicated_allocation",
		"VK_KHR_create_renderpass2",
		"VK_KHR_draw_indirect_count",
		"VK_KHR_imageless_framebuffer",
		"VK_KHR_uniform_buffer_standard_layout",
		"VK_EXT_descriptor_indexing",
		"VK_EXT_shader_viewport_index_layer",
		"VK_KHR_dynamic_rendering",
		"VK_KHR_synchronization2",
		"VK_EXT_extended_dynamic_state",
		"VK_EXT_extended_dynamic_state2",
	};
*/

#endif // defined(HAS_VULKAN) && !defined(HEADLESS)