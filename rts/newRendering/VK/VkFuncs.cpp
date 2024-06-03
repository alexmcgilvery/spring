#define VMA_IMPLEMENTATION

#include "System/Log/ILog.h"
#include "Game/GameVersion.h"

#include "VkFuncs.h"
#include "VkConfig.h"

#include <set>
#include <vector>
#include <string>

// Constant definitions
#ifdef NDEBUG
const bool 	ENABLE_VALIDATION_LAYERS = false;
#else
const bool  ENABLE_VALIDATION_LAYERS = true;
#endif

const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
};

void Vulkan::InitializeVulkanInstance(
	VkInstance&						_instance,
	VkAllocationCallbacks* 			_allocator_ptr,
	VkDebugUtilsMessengerEXT& 		_message_back,
	std::vector<const char*> 		_instance_extensions,
	std::vector<const char*> 		_instance_layers
	)
{
	if (ENABLE_VALIDATION_LAYERS && !CheckInstanceLayerSupport({VALIDATION_LAYERS})) {
		LOG("Validation layers requested but not available!");
	}

	// provide driver details about application + engine
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "spring";
	app_info.applicationVersion = std::stoi(SpringVersion::GetMajor()); // TODO
	app_info.pEngineName = "recoil";
	app_info.engineVersion = 0; // TODO
	app_info.apiVersion = Vulkan::RenderConfig::VK_API_VERSION;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = static_cast<uint32_t>(_instance_extensions.size());
	create_info.ppEnabledExtensionNames = _instance_extensions.data();

	if (ENABLE_VALIDATION_LAYERS) {
		_instance_layers.insert(_instance_layers.end(), VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
	}

	create_info.enabledLayerCount = static_cast<uint32_t>(_instance_layers.size());
	create_info.ppEnabledLayerNames = _instance_layers.data();

	// call vulkan create instance and store resulting instance in instance, success state of this is stored in result
	if (vkCreateInstance(&create_info, _allocator_ptr, &_instance) != VK_SUCCESS) {
		LOG("Failed to create instance!");
	}

	if (ENABLE_VALIDATION_LAYERS) {
		Vulkan::Validation::SetupDebugUtilsMessenger(_instance, _allocator_ptr, _message_back);
	}
}

void Vulkan::InitializeVulkanDevice(
	const VkPhysicalDevice& 		_physical_device, 
	QueueFamilyIndices& 			_indices, 
	VkDevice& 						_logical_device,  
	const VkSurfaceKHR				_surface,
	std::vector<const char*> 		_device_extensions,
	std::vector<const char*>		_layers
	)
{
	// specifing the queues to be created
	_indices = Vulkan::DeviceSupport::DetermineQueueFamilies(_physical_device, _surface);

	std::vector<VkDeviceQueueCreateInfo> queue_creation_infos;
	std::set<int> queue_families = { _indices.graphics, _indices.present, _indices.transfer, _indices.compute };

	float queue_priority = 1.0f;
	for (int queue_index : queue_families) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_index;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		queue_creation_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features = {}; // specifing used device features
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo logical_device_create_info = {};
	logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	logical_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_creation_infos.size());
	logical_device_create_info.pQueueCreateInfos = queue_creation_infos.data();

	logical_device_create_info.pEnabledFeatures = &device_features;
		
	// device specific extensions and validation layers
	logical_device_create_info.enabledExtensionCount = static_cast<uint32_t>(_device_extensions.size());
	logical_device_create_info.ppEnabledExtensionNames = _device_extensions.data();

	if (ENABLE_VALIDATION_LAYERS) {
		_layers.insert(_layers.end(), VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
	}

	logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(_layers.size());
	logical_device_create_info.ppEnabledLayerNames = _layers.data();


	if (vkCreateDevice(_physical_device, &logical_device_create_info, nullptr, &_logical_device) != VK_SUCCESS) {
		LOG("Failed to create logical device");
	}
}

void Vulkan::InitializeVulkanDescriptorPool(
	const VkDevice& 				_logical_device, 
	const VkAllocationCallbacks* 	_allocator_ptr, 
	VkDescriptorPool& 				_descriptor_pool
	)
{
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	}; //FIXME Working hack by just using high numbers of pool sizes, ideally would be calculated and sized appropriately

	int ArraySize = ((int)(sizeof(pool_sizes) / sizeof(*pool_sizes)));

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * ArraySize;
	pool_info.poolSizeCount = (uint32_t) ArraySize;
	pool_info.pPoolSizes = pool_sizes;

	vkCreateDescriptorPool(_logical_device, &pool_info, _allocator_ptr, &_descriptor_pool);	
}

void Vulkan::TerminateVulkanInstance(
	const VkInstance&				_instance,
	const VkAllocationCallbacks*	_allocator_ptr,
	VkDebugUtilsMessengerEXT&		_message_back
	)
{
	if (ENABLE_VALIDATION_LAYERS){
		Vulkan::Validation::DestroyDebugUtilsMessengerEXT(_instance, _allocator_ptr, _message_back);
	}

	vkDestroyInstance(_instance, _allocator_ptr);
}

std::vector<VkPhysicalDevice> Vulkan::DeterminePhysicalDevices(
	const VkInstance& 				_instance,
	std::vector<const char*>		_required_extensions,
	std::vector<const char*>		_required_layers
	)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(_instance, &device_count, nullptr); // count of devices that support vulkan

	if (device_count == 0) {
		LOG("Failed to find a GPU with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(device_count); // create an array of the supported devices (size of available devices)
	vkEnumeratePhysicalDevices(_instance, &device_count, devices.data()); // call vulkan driver to return list of devices that are supported

	std::vector<VkPhysicalDevice> suitable_devices;

	for (const auto& device : devices) {
		if (DeviceSupport::CheckDeviceExtensionSupport(device, _required_extensions)) {
			suitable_devices.push_back(device);
		}
	}

	if (suitable_devices.size() == 0) {
		LOG("Failed to find a suitable GPU! (missing extensions)");
	}

	return suitable_devices;
}

QueueFamilyIndices Vulkan::DeviceSupport::DetermineQueueFamilies(
	const VkPhysicalDevice& 		_physical_device, 
	const VkSurfaceKHR& 			_surface
	)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, queue_families.data());

	int index = 0;
	
	// TODO prioritize queue families that only support one type
	for (const auto& queue_family : queue_families) {
		if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			VkBool32 present_support = false;

			if (_surface != VK_NULL_HANDLE){
				// if the physical device supports presenting images to our KHR surface
				vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, index, _surface, &present_support); 
			}

			indices.graphics = index;

			if (queue_family.queueCount > 0 && present_support) {
				indices.present = index;
			}
		}

		if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT){
			indices.transfer = index;
		}

		if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT){
			indices.compute = index;
		}

		index++;
	}
	return indices;
}

std::vector<VkImage> Vulkan::InitializeVulkanSwapChainForSDL(
	const VkPhysicalDevice 			_physical_device, 
	const VkDevice 					_logical_device, 
	const VkAllocationCallbacks* 	_allocator_ptr, 
	const VkSurfaceKHR& 			_surface,  
	SDL_Window* const 				_window_ptr, 
	VkSwapchainKHR& 				_swap_chain, 
	VkFormat& 						_image_format, 
	VkExtent2D& 					_image_extent
	)
{
	//create Swapchain + Swapchain Images
	{
		using DeviceSupport::DeviceSwapChainSupportDetails;
		DeviceSwapChainSupportDetails swap_chain_support_details = DeviceSupport::QueryDeviceSwapChainSupport(_physical_device, _surface); // get the details of swap chain support from the physical device we've chosen

		VkSurfaceFormatKHR surface_format = DeviceSupport::ChooseSwapSurfaceFormat(swap_chain_support_details.formats);
		VkPresentModeKHR present_mode = DeviceSupport::ChooseSwapPresentMode(swap_chain_support_details.present_modes);
		VkExtent2D extent = DeviceSupport::ChooseSwapExtent(swap_chain_support_details.capabilities, _window_ptr);

		uint32_t image_count = swap_chain_support_details.capabilities.minImageCount + 1; // the number of images in the swap chain (length of the queue basically)
		if (swap_chain_support_details.capabilities.maxImageCount > 0 && image_count > swap_chain_support_details.capabilities.maxImageCount) { // value of 0 for maxImageCount means no limit beyond memory
			image_count = swap_chain_support_details.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = _surface;
		create_info.minImageCount = image_count;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = extent;
		create_info.imageArrayLayers = 1; // number of layers per image
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // the kind of operations the images in the swap chain will be used for

		QueueFamilyIndices indices = DeviceSupport::DetermineQueueFamilies(_physical_device, _surface);

		uint32_t queue_indicies[] = { (uint32_t) indices.graphics, (uint32_t) indices.present};

		if (indices.graphics != indices.present) {
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_indicies;
		}
		else {
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0; // Optional
			create_info.pQueueFamilyIndices = nullptr; // Optional
		}

		create_info.preTransform = swap_chain_support_details.capabilities.currentTransform; // a transformation that is auto-applied when images are added to swap chain, currentTransform is no transformation

		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore the alpha channel

		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;

		create_info.oldSwapchain = VK_NULL_HANDLE; // prior swap chain

		if (vkCreateSwapchainKHR(_logical_device, &create_info, _allocator_ptr, &_swap_chain) != VK_SUCCESS) { // create the swap chain
			LOG("failed to create swap chain!");
		}

		//Get the pointers to swapchain images
		vkGetSwapchainImagesKHR(_logical_device, _swap_chain, &image_count, nullptr); 
		std::vector<VkImage> swapchain_images(image_count);
		vkGetSwapchainImagesKHR(_logical_device, _swap_chain, &image_count, swapchain_images.data()); // add the handles of all the images to the list

		_image_format = surface_format.format;
		_image_extent = extent;

		return swapchain_images;
	}
}

void Vulkan::InitializeVulkanRenderPass(
	const VkDevice& 			_device, 
	const VkFormat& 			_format, 
	VkRenderPass& 				_render_pass
	)
{
	VkAttachmentDescription colour_attachment = {}; // colour buffer attachment represented by one of the images from the swap chain
	colour_attachment.format = _format; // format of the attachment
	colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT; // multisampling, no multisampling is 1 sample

	// for colour and depth data
	colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what we do with the data in the attachment before rendering (CLEAR to a constant, LOAD preserve contents, DONT_CARE we dont care about current)
	colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // what we do with the data in the attachment after rendering (STORE the contents in the attachment, DONT_CARE we don't care about the contents)

	// for stencil data
	colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // what we do with stencil data in the attachment before rendering
	colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // what we do with stencil data in the attachment after rendering

	// layout of pixels in memory can be changed based on what you want to do with the image
	colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout the image will have before the render pass begins
	colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout the image will have after the render pass finishes (PRESENT_SRC_KHR to get image ready for presentating in the swap chain)

	// single rendering pass can consist of multiple subpasses (subsequent rendering operations that depend on the contents of framebuffers in the previous passes)
	VkAttachmentReference colour_attachment_reference = {}; // every subpass references one or more of the attachments that we've described prior (VkAttachmentDescription), each reference is in the form of a VKAttachmentReference
	colour_attachment_reference.attachment = 0; // index of the attachment that we're applying the subpass to
	colour_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout we'd like the attachment to have during a subpass

	VkSubpassDescription subpass = {}; // describing the subpass 
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // this is a graphics subpass
	subpass.colorAttachmentCount = 1; // how many colour attachments there are 
	subpass.pColorAttachments = &colour_attachment_reference; // reference to the colour attachment for this subpass

	VkRenderPassCreateInfo renderPassInfo = {}; // the final render pass object based off of prior configurations
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1; // how many attachments in this render pass
	renderPassInfo.pAttachments = &colour_attachment; // the attachments pointer for this render pass
	renderPassInfo.subpassCount = 1; // how many subpasses in this render pass
	renderPassInfo.pSubpasses = &subpass; // the pointer to the subpasses in the render pass

	VkSubpassDependency dependency = {}; // describing a dependancy that we have on the subpass
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0; // the subpass we depend on to do a render pass
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // the operation to wait on
	dependency.srcAccessMask = 0; // the stage in which this operation occurs
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // wait on the colour attachment output stage
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // wait for the reading and writing stage to be finished

	renderPassInfo.dependencyCount = 1; // how many dependencies we have 
	renderPassInfo.pDependencies = &dependency; // the array of dependencies

	if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_render_pass) != VK_SUCCESS) { // creating the render pass and storing it in our global variable
		LOG("failed to create render pass!");
	}
}

//Pipeline
//Required: shaders + descriptor layout + renderpass
void Vulkan::InitializeGraphicsPipeline( //FIXME Mod to match BAR requirements
	const VkDevice& 				_device, 
	const VkPipelineCache& 			_pipeline_cache, 
	const VkAllocationCallbacks*  	_allocator_ptr, 
	const VkRenderPass& 			_render_pass, 
	const VkExtent2D 				_extent,  
	VkPipelineLayout& 				_pipeline_layout, 
	VkDescriptorSetLayout& 			_descriptor_set_layout, 
	VkPipeline& 					_pipeline
	)
{
	VkResult err;
	VkShaderModule vert_module;
	VkShaderModule frag_module;

	// Create The Shader Modules:
	{
		VkShaderModuleCreateInfo vert_info = {};
		vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vert_info.codeSize = 4; //sizeof(object_shader_vert_spv);
		vert_info.pCode = (uint32_t*)nullptr; //FIXME
		err = vkCreateShaderModule(_device, &vert_info, _allocator_ptr, &vert_module);

		VkShaderModuleCreateInfo frag_info = {};
		frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		frag_info.codeSize = 4; //sizeof(object_shader_frag_spv);
		frag_info.pCode = (uint32_t*)nullptr; //FIXME
		err = vkCreateShaderModule(_device, &frag_info, _allocator_ptr, &frag_module);
	}

	// Create a descriptor set layout for textures
	{
        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bindings[1] = {samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = bindings;

        if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptor_set_layout) != VK_SUCCESS) {
            LOG("failed to create texture descriptor set layout!");
        }
	}

	//create pipeline layout
	if (_pipeline_layout == VK_NULL_HANDLE)
	{
		VkPipelineLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount = 1;
		layout_info.pSetLayouts = &_descriptor_set_layout;
		layout_info.pushConstantRangeCount = 0;
		layout_info.pPushConstantRanges = nullptr;
		err = vkCreatePipelineLayout(_device, &layout_info, _allocator_ptr, & _pipeline_layout);
	}

	// Create the pipeline
	{
		VkPipelineShaderStageCreateInfo stage[2] = {};
		stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage[0].module = vert_module;
		stage[0].pName = "main";
		stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage[1].module = frag_module;
		stage[1].pName = "main";

		VkVertexInputBindingDescription binding_desc[1] = {};
		binding_desc[0].binding = 0;
		binding_desc[0].stride = 4; // sizeof(glm::vec4);
		binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attribute_desc[2] = {};
		attribute_desc[0].location = 0;
		attribute_desc[0].binding = binding_desc[0].binding;
		attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_desc[0].offset = 0;
		attribute_desc[1].location = 1;
		attribute_desc[1].binding = binding_desc[0].binding;
		attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_desc[1].offset = 4; //sizeof(glm::vec2);

		VkPipelineVertexInputStateCreateInfo vertex_info = {};
		vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_info.vertexBindingDescriptionCount = 1;
		vertex_info.pVertexBindingDescriptions = binding_desc;
		vertex_info.vertexAttributeDescriptionCount = 2;
		vertex_info.pVertexAttributeDescriptions = attribute_desc;

		VkPipelineInputAssemblyStateCreateInfo ia_info = {};
		ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;


		VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) _extent.width;
        viewport.height = (float) _extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = _extent;

        VkPipelineViewportStateCreateInfo viewport_info = {};
        viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_info.viewportCount = 1;
        viewport_info.pViewports = &viewport;
        viewport_info.scissorCount = 1;
        viewport_info.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo raster_info = {};
		raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster_info.polygonMode = VK_POLYGON_MODE_FILL;
		raster_info.cullMode = VK_CULL_MODE_NONE;
		raster_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		raster_info.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo ms_info = {};
		ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState color_attachment[1] = {};
		color_attachment[0].blendEnable = VK_TRUE;
		color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
		color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
		color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineDepthStencilStateCreateInfo depth_info = {};
		depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkPipelineColorBlendStateCreateInfo blend_info = {};
		blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blend_info.attachmentCount = 1;
		blend_info.pAttachments = color_attachment;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.flags = 0;
		info.stageCount = 2;
		info.pStages = stage;
		info.pVertexInputState = &vertex_info;
		info.pInputAssemblyState = &ia_info;
		info.pViewportState = &viewport_info;
		info.pRasterizationState = &raster_info;
		info.pMultisampleState = &ms_info;
		info.pDepthStencilState = &depth_info;
		info.pColorBlendState = &blend_info;
		info.layout =  _pipeline_layout;
		info.renderPass = _render_pass;
		err = vkCreateGraphicsPipelines(_device, _pipeline_cache, 1, &info, _allocator_ptr, &_pipeline);
	}

	vkDestroyShaderModule(_device, vert_module, _allocator_ptr);
	vkDestroyShaderModule(_device, frag_module, _allocator_ptr);
}


bool Vulkan::DeviceSupport::CheckDeviceExtensionSupport(
	const VkPhysicalDevice& 		_physical_device, 
	std::vector<const char*> 		_required_extensions
	)
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(_physical_device, nullptr, &extension_count, nullptr); // query the vulkan driver to check how many extensions are supported by the specified physical device

	std::vector<VkExtensionProperties> available_extensions(extension_count); // based off how many extensions are supported in the prior statement create a list of that size
	vkEnumerateDeviceExtensionProperties(_physical_device, nullptr, &extension_count, available_extensions.data()); // fill the list with the supported extensions

	std::set<std::string> required_extensions(_required_extensions.begin(), _required_extensions.end()); // create a list of the required extensions

	for (const auto &extension : available_extensions) { // for every extension in the available extensions from the device
		required_extensions.erase(extension.extensionName); // if the extension is in the required extension list, remove it from the requried list (as we know its available)
	}

	return required_extensions.empty(); // the list should be empty if all the required extensions are found, if it is empty we are returning true here
}

bool Vulkan::CheckInstanceLayerSupport(
	std::vector<const char*>			_required_layers
	)
{
    uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	//loop through all of the validation layers
	for (const char* layer_name : _required_layers) {
		bool layer_found = false;

		for (const auto& layerProperties : available_layers) {
			if (strcmp(layer_name, layerProperties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) {
			return false;
		}
	}

	return true;
}

VkResult Vulkan::Data::CreateOrResizeBuffer(
	const VmaAllocator&					_vma_allocator,
	const VkBufferCreateInfo&			_buffer_create_info,
	const VmaMemoryUsage&				_memory_usage,
	VkBuffer&							_buffer,
	VmaAllocation&						_memory_allocation
	)
{
	VmaAllocationCreateInfo buffer_alloc_info = {};
	buffer_alloc_info.usage = _memory_usage;

	if (_buffer != VK_NULL_HANDLE && _memory_allocation != VMA_NULL){
		vmaDestroyBuffer(_vma_allocator, _buffer, _memory_allocation);
	}
	return vmaCreateBuffer(_vma_allocator, &_buffer_create_info, &buffer_alloc_info, &_buffer, &_memory_allocation, nullptr);
}

VkResult Vulkan::Data::CreateOrResizeImage(
	const VmaAllocator&					_vma_allocator,
	const VkImageCreateInfo&			_image_create_info,
	const VmaMemoryUsage&				_memory_usage,
	VkImage&							_image,
	VmaAllocation&						_memory_allocation
	)
{
	VmaAllocationCreateInfo image_alloc_info = {};
	image_alloc_info.usage = _memory_usage;

	if (_image != VK_NULL_HANDLE && _memory_allocation != VMA_NULL){
		vmaDestroyImage(_vma_allocator, _image, _memory_allocation);
	}
	return vmaCreateImage(_vma_allocator, &_image_create_info, &image_alloc_info, &_image, &_memory_allocation, nullptr);
}

void Vulkan::Validation::SetupDebugUtilsMessenger(
	const VkInstance& 				_instance, 
	const VkAllocationCallbacks* 	_allocator_ptr, 
	VkDebugUtilsMessengerEXT& 		_message_back
	)
{
    if (!ENABLE_VALIDATION_LAYERS) return;

	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = DebugMessengerCallback;

	if (Validation::CreateDebugUtilsMessengerEXT(_instance, _allocator_ptr, &create_info, _message_back) != VK_SUCCESS) {
		LOG("Failed to set up debug callback!");
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::Validation::DebugMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT 		_message_severity, 
	VkDebugUtilsMessageTypeFlagsEXT 			_message_type, 
	const VkDebugUtilsMessengerCallbackDataEXT* _callback_data, 
	void* 										_user_data
	) 
{
	std::string output("Validation layer: ");
	output +=  _callback_data->pMessage;
	LOG(output.c_str());

	return VK_FALSE;
}

VkResult Vulkan::Validation::CreateDebugUtilsMessengerEXT(
	const VkInstance& 							_instance, 
	const VkAllocationCallbacks* 				_allocator_ptr, 
	const VkDebugUtilsMessengerCreateInfoEXT* 	_create_info_ptr, 
	VkDebugUtilsMessengerEXT& 					_message_back
	) 
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(_instance, _create_info_ptr, _allocator_ptr, &_message_back);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Vulkan::Validation::DestroyDebugUtilsMessengerEXT(
	const VkInstance& 							_instance, 
	const VkAllocationCallbacks* 				_allocator_ptr, 
	VkDebugUtilsMessengerEXT& 					_message_back
	) 
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(_instance, _message_back, _allocator_ptr);
	}
}

std::vector<const char*> Vulkan::GetRequiredSDLExtensions(SDL_Window* _window_ptr)
{
	uint32_t sdl_extension_count = 0;
	const char** sdl_extensions;
	SDL_Vulkan_GetInstanceExtensions(_window_ptr, &sdl_extension_count, sdl_extensions);

	std::vector<const char*> extensions(sdl_extensions, sdl_extensions + sdl_extension_count);

	if (ENABLE_VALIDATION_LAYERS) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

Vulkan::DeviceSupport::DeviceSwapChainSupportDetails Vulkan::DeviceSupport::QueryDeviceSwapChainSupport(
	const VkPhysicalDevice& 	_physical_device, 
	const VkSurfaceKHR& 		_surface
	)
{
	using DeviceSupport::DeviceSwapChainSupportDetails;

	DeviceSwapChainSupportDetails details;
	
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &details.capabilities); // fill in the result of checking if the physical device has KHR surface capabilities

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &format_count, nullptr);

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

VkSurfaceFormatKHR Vulkan::DeviceSupport::ChooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& _available_formats
	)
{
	if (_available_formats.size() == 1 && _available_formats[0].format == VK_FORMAT_UNDEFINED) { // if the surface has no prefered format then we choose
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }; // set the format to RGB with Alpha layer 8 bits per
	}

	for (const auto& available_format : _available_formats) {
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return available_format; // if multiple supported, look for what we want (B8G8R8A8) and return if found
		}
	}

	return _available_formats[0]; // if multiple supported and we don't find what we want just choose the first one (we might want to look for the best as an alternative to this)
}

VkPresentModeKHR Vulkan::DeviceSupport::ChooseSwapPresentMode(
	const std::vector<VkPresentModeKHR> _available_present_modes
	)
{
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR; // Last Priority

	for (const auto& present_mode : _available_present_modes) {
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) { // Priority
			return present_mode;
		}
		else if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) { // Second Priority
			best_mode = present_mode;
		}
	}

	return best_mode;
}

VkExtent2D Vulkan::DeviceSupport::ChooseSwapExtent(
	const VkSurfaceCapabilitiesKHR& 	_capabilities, 
	SDL_Window* const					_window
	)
{
	if (_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return _capabilities.currentExtent;
	}
	else {
		int width;
		int height;
		SDL_GetWindowSize(_window, &width, &height);
		VkExtent2D actual_extent = { (uint32_t) width, (uint32_t) height };

		actual_extent.width = std::max(_capabilities.minImageExtent.width, std::min(_capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(_capabilities.minImageExtent.height, std::min(_capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}