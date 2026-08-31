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

void vulkan::InitializeVulkanInstance(
	VkInstance&						instance,
	VkAllocationCallbacks* 			allocatorCallbacks,
	VkDebugUtilsMessengerEXT& 		debugMessenger,
	std::vector<const char*> 		instanceExtensions,
	std::vector<const char*> 		instanceLayers
	)
{
	if (ENABLE_VALIDATION_LAYERS && !CheckInstanceLayerSupport({VALIDATION_LAYERS})) {
		LOG("Validation layers requested but not available!");
	}

	// provide driver details about application + engine
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "spring";
	appInfo.applicationVersion = std::stoi(SpringVersion::GetMajor()); // TODO
	appInfo.pEngineName = "recoil";
	appInfo.engineVersion = 0; // TODO
	appInfo.apiVersion = vulkan::render_config::VK_API_VERSION;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	if (ENABLE_VALIDATION_LAYERS) {
		instanceLayers.insert(instanceLayers.end(), VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
	}

	createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
	createInfo.ppEnabledLayerNames = instanceLayers.data();

	// call vulkan create instance and store resulting instance in instance, success state of this is stored in result
	if (vkCreateInstance(&createInfo, allocatorCallbacks, &instance) != VK_SUCCESS) {
		LOG("Failed to create instance!");
	}

	if (ENABLE_VALIDATION_LAYERS) {
		vulkan::validation::SetupDebugUtilsMessenger(instance, allocatorCallbacks, debugMessenger);
	}
}

void vulkan::InitializeVulkanDevice(
	const VkPhysicalDevice& 		physicalDevice,
	QueueFamilyIndices& 			queueFamilyIndices,
	VkDevice& 						logicalDevice,
	const VkSurfaceKHR				surface,
	std::vector<const char*> 		deviceExtensions,
	std::vector<const char*>		layers
	)
{
	// specifing the queues to be created
	queueFamilyIndices = vulkan::device_support::DetermineQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilies = { queueFamilyIndices.graphics, queueFamilyIndices.present, queueFamilyIndices.transfer, queueFamilyIndices.compute };

	float queuePriority = 1.0f;
	for (int queueIndex : queueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {}; // specifing used device features
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo logicalDeviceCreateInfo = {};
	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	logicalDeviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// device specific extensions and validation layers
	logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	logicalDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (ENABLE_VALIDATION_LAYERS) {
		layers.insert(layers.end(), VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
	}

	logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	logicalDeviceCreateInfo.ppEnabledLayerNames = layers.data();


	if (vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
		LOG("Failed to create logical device");
	}
}

void vulkan::InitializeVulkanDescriptorPool(
	const VkDevice& 				logicalDevice,
	const VkAllocationCallbacks* 	allocatorCallbacks,
	VkDescriptorPool& 				descriptorPool
	)
{
	VkDescriptorPoolSize poolSizes[] =
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

	int arraySize = ((int)(sizeof(poolSizes) / sizeof(*poolSizes)));

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000 * arraySize;
	poolInfo.poolSizeCount = (uint32_t) arraySize;
	poolInfo.pPoolSizes = poolSizes;

	vkCreateDescriptorPool(logicalDevice, &poolInfo, allocatorCallbacks, &descriptorPool);
}

void vulkan::TerminateVulkanInstance(
	const VkInstance&				instance,
	const VkAllocationCallbacks*	allocatorCallbacks,
	VkDebugUtilsMessengerEXT&		debugMessenger
	)
{
	if (ENABLE_VALIDATION_LAYERS){
		vulkan::validation::DestroyDebugUtilsMessengerEXT(instance, allocatorCallbacks, debugMessenger);
	}

	vkDestroyInstance(instance, allocatorCallbacks);
}

std::vector<VkPhysicalDevice> vulkan::DeterminePhysicalDevices(
	const VkInstance& 				instance,
	std::vector<const char*>		requiredExtensions,
	std::vector<const char*>		requiredLayers
	)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); // count of devices that support vulkan

	if (deviceCount == 0) {
		LOG("Failed to find a GPU with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount); // create an array of the supported devices (size of available devices)
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()); // call vulkan driver to return list of devices that are supported

	std::vector<VkPhysicalDevice> suitableDevices;

	for (const auto& device : devices) {
		if (device_support::CheckDeviceExtensionSupport(device, requiredExtensions)) {
			suitableDevices.push_back(device);
		}
	}

	if (suitableDevices.size() == 0) {
		LOG("Failed to find a suitable GPU! (missing extensions)");
	}

	return suitableDevices;
}

QueueFamilyIndices vulkan::device_support::DetermineQueueFamilies(
	const VkPhysicalDevice& 		physicalDevice,
	const VkSurfaceKHR& 			surface
	)
{
	QueueFamilyIndices queueFamilyIndices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int index = 0;

	// TODO prioritize queue families that only support one type
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			VkBool32 presentSupport = false;

			if (surface != VK_NULL_HANDLE){
				// if the physical device supports presenting images to our KHR surface
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport);
			}

			queueFamilyIndices.graphics = index;

			if (queueFamily.queueCount > 0 && presentSupport) {
				queueFamilyIndices.present = index;
			}
		}

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT){
			queueFamilyIndices.transfer = index;
		}

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT){
			queueFamilyIndices.compute = index;
		}

		index++;
	}
	return queueFamilyIndices;
}

std::vector<VkImage> vulkan::InitializeVulkanSwapchainForSDL(
	const VkPhysicalDevice 			physicalDevice,
	const VkDevice 					logicalDevice,
	const VkAllocationCallbacks* 	allocatorCallbacks,
	const VkSurfaceKHR& 			surface,
	SDL_Window* const 				window,
	VkSwapchainKHR& 				swapchain,
	VkFormat& 						imageFormat,
	VkExtent2D& 					imageExtent
	)
{
	//create Swapchain + Swapchain Images
	{
		using device_support::DeviceSwapchainSupportDetails;
		DeviceSwapchainSupportDetails swapchainSupportDetails = device_support::QueryDeviceSwapchainSupport(physicalDevice, surface); // get the details of swap chain support from the physical device we've chosen

		VkSurfaceFormatKHR surfaceFormat = device_support::ChooseSwapSurfaceFormat(swapchainSupportDetails.formats);
		VkPresentModeKHR presentMode = device_support::ChooseSwapPresentMode(swapchainSupportDetails.presentModes);
		VkExtent2D extent = device_support::ChooseSwapExtent(swapchainSupportDetails.capabilities, window);

		uint32_t imageCount = swapchainSupportDetails.capabilities.minImageCount + 1; // the number of images in the swap chain (length of the queue basically)
		if (swapchainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.capabilities.maxImageCount) { // value of 0 for maxImageCount means no limit beyond memory
			imageCount = swapchainSupportDetails.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // number of layers per image
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // the kind of operations the images in the swap chain will be used for

		QueueFamilyIndices queueFamilyIndices = device_support::DetermineQueueFamilies(physicalDevice, surface);

		uint32_t queueFamilyIndexArray[] = { (uint32_t) queueFamilyIndices.graphics, (uint32_t) queueFamilyIndices.present};

		if (queueFamilyIndices.graphics != queueFamilyIndices.present) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndexArray;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapchainSupportDetails.capabilities.currentTransform; // a transformation that is auto-applied when images are added to swap chain, currentTransform is no transformation

		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore the alpha channel

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE; // prior swap chain

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, allocatorCallbacks, &swapchain) != VK_SUCCESS) { // create the swap chain
			LOG("failed to create swap chain!");
		}

		//Get the pointers to swapchain images
		vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
		std::vector<VkImage> swapchainImages(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, swapchainImages.data()); // add the handles of all the images to the list

		imageFormat = surfaceFormat.format;
		imageExtent = extent;

		return swapchainImages;
	}
}

void vulkan::InitializeVulkanRenderPass(
	const VkDevice& 			device,
	const VkFormat& 			format,
	VkRenderPass& 				renderPass
	)
{
	VkAttachmentDescription colorAttachment = {}; // colour buffer attachment represented by one of the images from the swap chain
	colorAttachment.format = format; // format of the attachment
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // multisampling, no multisampling is 1 sample

	// for colour and depth data
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what we do with the data in the attachment before rendering (CLEAR to a constant, LOAD preserve contents, DONT_CARE we dont care about current)
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // what we do with the data in the attachment after rendering (STORE the contents in the attachment, DONT_CARE we don't care about the contents)

	// for stencil data
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // what we do with stencil data in the attachment before rendering
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // what we do with stencil data in the attachment after rendering

	// layout of pixels in memory can be changed based on what you want to do with the image
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout the image will have before the render pass begins
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout the image will have after the render pass finishes (PRESENT_SRC_KHR to get image ready for presentating in the swap chain)

	// single rendering pass can consist of multiple subpasses (subsequent rendering operations that depend on the contents of framebuffers in the previous passes)
	VkAttachmentReference colorAttachmentReference = {}; // every subpass references one or more of the attachments that we've described prior (VkAttachmentDescription), each reference is in the form of a VKAttachmentReference
	colorAttachmentReference.attachment = 0; // index of the attachment that we're applying the subpass to
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout we'd like the attachment to have during a subpass

	VkSubpassDescription subpass = {}; // describing the subpass
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // this is a graphics subpass
	subpass.colorAttachmentCount = 1; // how many colour attachments there are
	subpass.pColorAttachments = &colorAttachmentReference; // reference to the colour attachment for this subpass

	VkRenderPassCreateInfo renderPassInfo = {}; // the final render pass object based off of prior configurations
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1; // how many attachments in this render pass
	renderPassInfo.pAttachments = &colorAttachment; // the attachments pointer for this render pass
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

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) { // creating the render pass and storing it in our global variable
		LOG("failed to create render pass!");
	}
}

//Pipeline
//Required: shaders + descriptor layout + renderpass
void vulkan::InitializeGraphicsPipeline( //FIXME Mod to match BAR requirements
	const VkDevice& 				device,
	const VkPipelineCache& 			pipelineCache,
	const VkAllocationCallbacks*  	allocatorCallbacks,
	const VkRenderPass& 			renderPass,
	const VkExtent2D 				extent,
	VkPipelineLayout& 				pipelineLayout,
	VkDescriptorSetLayout& 			descriptorSetLayout,
	VkPipeline& 					pipeline
	)
{
	VkResult result;
	VkShaderModule vertexShaderModule;
	VkShaderModule fragmentShaderModule;

	// Create The Shader Modules:
	{
		VkShaderModuleCreateInfo vertexShaderInfo = {};
		vertexShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vertexShaderInfo.codeSize = 4; //sizeof(object_shader_vert_spv);
		vertexShaderInfo.pCode = (uint32_t*)nullptr; //FIXME
		result = vkCreateShaderModule(device, &vertexShaderInfo, allocatorCallbacks, &vertexShaderModule);

		VkShaderModuleCreateInfo fragmentShaderInfo = {};
		fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fragmentShaderInfo.codeSize = 4; //sizeof(object_shader_frag_spv);
		fragmentShaderInfo.pCode = (uint32_t*)nullptr; //FIXME
		result = vkCreateShaderModule(device, &fragmentShaderInfo, allocatorCallbacks, &fragmentShaderModule);
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

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			LOG("failed to create texture descriptor set layout!");
		}
	}

	//create pipeline layout
	if (pipelineLayout == VK_NULL_HANDLE)
	{
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = 1;
		layoutInfo.pSetLayouts = &descriptorSetLayout;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;
		result = vkCreatePipelineLayout(device, &layoutInfo, allocatorCallbacks, & pipelineLayout);
	}

	// Create the pipeline
	{
		VkPipelineShaderStageCreateInfo shaderStages[2] = {};
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertexShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = fragmentShaderModule;
		shaderStages[1].pName = "main";

		VkVertexInputBindingDescription bindingDescriptions[1] = {};
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = 4; // sizeof(glm::vec4);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attributeDescriptions[2] = {};
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].binding = bindingDescriptions[0].binding;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].binding = bindingDescriptions[0].binding;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = 4; //sizeof(glm::vec2);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions;
		vertexInputInfo.vertexAttributeDescriptionCount = 2;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;


		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) extent.width;
		viewport.height = (float) extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = extent;

		VkPipelineViewportStateCreateInfo viewportInfo = {};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorAttachment[1] = {};
		colorAttachment[0].blendEnable = VK_TRUE;
		colorAttachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorAttachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorAttachment[0].colorBlendOp = VK_BLEND_OP_ADD;
		colorAttachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorAttachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorAttachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
		colorAttachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineDepthStencilStateCreateInfo depthInfo = {};
		depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkPipelineColorBlendStateCreateInfo blendInfo = {};
		blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendInfo.attachmentCount = 1;
		blendInfo.pAttachments = colorAttachment;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.flags = 0;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportInfo;
		pipelineInfo.pRasterizationState = &rasterizationInfo;
		pipelineInfo.pMultisampleState = &multisampleInfo;
		pipelineInfo.pDepthStencilState = &depthInfo;
		pipelineInfo.pColorBlendState = &blendInfo;
		pipelineInfo.layout =  pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		result = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, allocatorCallbacks, &pipeline);
	}

	vkDestroyShaderModule(device, vertexShaderModule, allocatorCallbacks);
	vkDestroyShaderModule(device, fragmentShaderModule, allocatorCallbacks);
}


bool vulkan::device_support::CheckDeviceExtensionSupport(
	const VkPhysicalDevice& 		physicalDevice,
	std::vector<const char*> 		requiredExtensions
	)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr); // query the vulkan driver to check how many extensions are supported by the specified physical device

	std::vector<VkExtensionProperties> availableExtensions(extensionCount); // based off how many extensions are supported in the prior statement create a list of that size
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data()); // fill the list with the supported extensions

	std::set<std::string> requiredExtensions(requiredExtensions.begin(), requiredExtensions.end()); // create a list of the required extensions

	for (const auto &extension : availableExtensions) { // for every extension in the available extensions from the device
		requiredExtensions.erase(extension.extensionName); // if the extension is in the required extension list, remove it from the requried list (as we know its available)
	}

	return requiredExtensions.empty(); // the list should be empty if all the required extensions are found, if it is empty we are returning true here
}

bool vulkan::CheckInstanceLayerSupport(
	std::vector<const char*>			requiredLayers
	)
{
    uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	//loop through all of the validation layers
	for (const char* layerName : requiredLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

VkResult vulkan::data::CreateOrResizeBuffer(
	const VmaAllocator&					vmaAllocator,
	const VkBufferCreateInfo&			bufferCreateInfo,
	const VmaMemoryUsage&				memoryUsage,
	VkBuffer&							buffer,
	VmaAllocation&						allocation
	)
{
	VmaAllocationCreateInfo bufferAllocationInfo = {};
	bufferAllocationInfo.usage = memoryUsage;

	if (buffer != VK_NULL_HANDLE && allocation != VMA_NULL){
		vmaDestroyBuffer(vmaAllocator, buffer, allocation);
	}
	return vmaCreateBuffer(vmaAllocator, &bufferCreateInfo, &bufferAllocationInfo, &buffer, &allocation, nullptr);
}

VkResult vulkan::data::CreateOrResizeImage(
	const VmaAllocator&					vmaAllocator,
	const VkImageCreateInfo&			imageCreateInfo,
	const VmaMemoryUsage&				memoryUsage,
	VkImage&							image,
	VmaAllocation&						allocation
	)
{
	VmaAllocationCreateInfo imageAllocationInfo = {};
	imageAllocationInfo.usage = memoryUsage;

	if (image != VK_NULL_HANDLE && allocation != VMA_NULL){
		vmaDestroyImage(vmaAllocator, image, allocation);
	}
	return vmaCreateImage(vmaAllocator, &imageCreateInfo, &imageAllocationInfo, &image, &allocation, nullptr);
}

void vulkan::validation::SetupDebugUtilsMessenger(
	const VkInstance& 				instance,
	const VkAllocationCallbacks* 	allocatorCallbacks,
	VkDebugUtilsMessengerEXT& 		debugMessenger
	)
{
    if (!ENABLE_VALIDATION_LAYERS) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugMessengerCallback;

	if (validation::CreateDebugUtilsMessengerEXT(instance, allocatorCallbacks, &createInfo, debugMessenger) != VK_SUCCESS) {
		LOG("Failed to set up debug callback!");
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan::validation::DebugMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT 		messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT 			messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* 										userData
	)
{
	std::string validationMessage("Validation layer: ");
	validationMessage +=  callbackData->pMessage;
	LOG(validationMessage.c_str());

	return VK_FALSE;
}

VkResult vulkan::validation::CreateDebugUtilsMessengerEXT(
	const VkInstance& 							instance,
	const VkAllocationCallbacks* 				allocatorCallbacks,
	const VkDebugUtilsMessengerCreateInfoEXT* 	createInfo,
	VkDebugUtilsMessengerEXT& 					debugMessenger
	)
{
	auto createDebugMessenger = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (createDebugMessenger != nullptr) {
		return createDebugMessenger(instance, createInfo, allocatorCallbacks, &debugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void vulkan::validation::DestroyDebugUtilsMessengerEXT(
	const VkInstance& 							instance,
	const VkAllocationCallbacks* 				allocatorCallbacks,
	VkDebugUtilsMessengerEXT& 					debugMessenger
	)
{
	auto destroyDebugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (destroyDebugMessenger != nullptr) {
		destroyDebugMessenger(instance, debugMessenger, allocatorCallbacks);
	}
}

std::vector<const char*> vulkan::GetRequiredSDLExtensions(SDL_Window* window)
{
	uint32_t sdlExtensionCount = 0;
	const char** sdlExtensions;
	SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions);

	std::vector<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

	if (ENABLE_VALIDATION_LAYERS) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

vulkan::device_support::DeviceSwapchainSupportDetails vulkan::device_support::QueryDeviceSwapchainSupport(
	const VkPhysicalDevice& 	physicalDevice,
	const VkSurfaceKHR& 		surface
	)
{
	using device_support::DeviceSwapchainSupportDetails;

	DeviceSwapchainSupportDetails swapchainSupportDetails;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchainSupportDetails.capabilities); // fill in the result of checking if the physical device has KHR surface capabilities

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		swapchainSupportDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapchainSupportDetails.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		swapchainSupportDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, swapchainSupportDetails.presentModes.data());
	}

	return swapchainSupportDetails;
}

VkSurfaceFormatKHR vulkan::device_support::ChooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& availableFormats
	)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) { // if the surface has no prefered format then we choose
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }; // set the format to RGB with Alpha layer 8 bits per
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat; // if multiple supported, look for what we want (B8G8R8A8) and return if found
		}
	}

	return availableFormats[0]; // if multiple supported and we don't find what we want just choose the first one (we might want to look for the best as an alternative to this)
}

VkPresentModeKHR vulkan::device_support::ChooseSwapPresentMode(
	const std::vector<VkPresentModeKHR> availableModes
	)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR; // Last Priority

	for (const auto& presentMode : availableModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) { // Priority
			return presentMode;
		}
		else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) { // Second Priority
			bestMode = presentMode;
		}
	}

	return bestMode;
}

VkExtent2D vulkan::device_support::ChooseSwapExtent(
	const VkSurfaceCapabilitiesKHR& 	capabilities,
	SDL_Window* const					window
	)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width;
		int height;
		SDL_GetWindowSize(window, &width, &height);
		VkExtent2D actualExtent = { (uint32_t) width, (uint32_t) height };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
