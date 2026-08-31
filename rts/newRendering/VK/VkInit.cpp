#if 0

#include "VkInit.h"
#include "VkFuncs.h"
#include "VkConfig.h"
#include "fmt/format.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <string_view>

/**
bool VkPhysicalDevicesCompare(const vk::PhysicalDevice& lhs, const vk::PhysicalDevice& rhs)
{
	const auto lhsDiscrete = static_cast<uint8_t>(lhs.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu);
	const auto rhsDiscrete = static_cast<uint8_t>(rhs.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu);

	if (lhsDiscrete > rhsDiscrete) return true;
	if (lhsDiscrete < rhsDiscrete) return false;

	VkDeviceSize lhsMemorySize = VkCoreObjects::GetVRAMSize(lhs);
	VkDeviceSize rhsMemorySize = VkCoreObjects::GetVRAMSize(rhs);

	if (lhsMemorySize > rhsMemorySize) return true;
	if (lhsMemorySize < rhsMemorySize) return false;

	return false;
};

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
VkCoreObjects::VkCoreObjects()
{
	vkInitialized = (SDL_Vulkan_LoadLibrary(nullptr) == 0);

	if (!vkInitialized)
		return;

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());

	vkInitialized &= (vkGetInstanceProcAddr != nullptr);
	if (!vkInitialized)
		return;

	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	// Determine what API version is available
	apiVersion = vk::enumerateInstanceVersion();

	vk::ApplicationInfo applicationInfo("spring", 1, "spring", 1, apiVersion);
	vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo);
	instance = vk::createInstance(instanceCreateInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

	instanceExtensionProperties = vk::enumerateInstanceExtensionProperties();

	physicalDevices = instance.enumeratePhysicalDevices();
	std::sort(physicalDevices.begin(), physicalDevices.end(), VkPhysicalDevicesCompare);

	physicalDevice = physicalDevices.front();
	physicalDeviceExtensionProperties = physicalDevice.enumerateDeviceExtensionProperties();

}

std::string VkCoreObjects1::GetVulkanAPI(uint32_t apiVersion)
{
	return fmt::format("{}.{}.{}", VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion), VK_API_VERSION_PATCH(apiVersion));
}

std::string VkCoreObjects::GetVulkanAPI() const
{
	return GetVulkanAPI(apiVersion);
}

VkDeviceSize VkCoreObjects::GetVRAMSize(const vk::PhysicalDevice& physicalDevice)
{
	VkDeviceSize memorySize = 0;
	const auto memoryProperties = physicalDevice.getMemoryProperties();
	for (auto heapIndex = 0; heapIndex < memoryProperties.memoryHeapCount; ++heapIndex) {
		if (memoryProperties.memoryHeaps[heapIndex].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
			memorySize += memoryProperties.memoryHeaps[heapIndex].size;
	}

	return memorySize;
}

bool VkCoreObjects::HasExtension(const std::vector<vk::ExtensionProperties>& extensions, const char* extensionName)
{
	return std::find_if(es.begin(), es.end(),
		[extensionName](const auto& item) {
			return std::string_view(item.extensionName).compare(extensionName) == 0;
		}
	) != es.end();
}

VkCoreObjects::~VkCoreObjects()
{
	if (!vkInitialized)
		return;

	instance.destroy();
	SDL_Vulkan_UnloadLibrary();
}

#endif

*/

VkCoreObjects::~VkCoreObjects()
{
	if (!vkInitialized)
		return;

	TerminateVulkanImages();
	TerminateVulkanSwapchainImages();
	TerminateVulkanGraphicsPipeline();
	TerminateVulkanRenderPass();
	TerminateVulkanSwapchainForSDL();
	TerminateVulkanSwapchainTaskControls();
	TerminateVulkanDevices();
	TerminateVulkanCore();

	SDL_Vulkan_UnloadLibrary();
}

const VkDeviceSize VkCoreObjects::GetCurrentDeviceVRAMSize()
{
	VkDeviceSize memorySize = 0;
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(vkDevice.physicalDevice, &memoryProperties);
	for (auto heapIndex = 0; heapIndex < memoryProperties.memoryHeapCount; ++heapIndex) {
		if (memoryProperties.memoryHeaps[heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			memorySize += memoryProperties.memoryHeaps[heapIndex].size;
	}

	return memorySize;
}

void VkCoreObjects::InitializeVulkanForSDL(SDL_Window* window)
{
	vulkan::InitializeVulkanInstance(vkCore.instance, vkCore.allocatorCallbacks, vkCore.debugMessenger, vulkan::GetRequiredSDLExtensions(window));

	SDL_bool result = SDL_Vulkan_CreateSurface(window, vkCore.instance, &vkWindow.surface);

	std::vector<VkPhysicalDevice> devices = vulkan::DeterminePhysicalDevices(vkCore.instance, vulkan::render_config::REQUIRED_DEVICE_EXTENSIONS);

	vkDevice.physicalDevice = devices[0];

	vulkan::InitializeVulkanDevice(vkDevice.physicalDevice, vkDevice.queueFamilyIndices, vkDevice.logicalDevice, vkWindow.surface,vulkan::render_config::REQUIRED_DEVICE_EXTENSIONS);

	vkGetPhysicalDeviceProperties(vkDevice.physicalDevice, &vkDevice.physicalDeviceProperties); // load device properties
	vkGetPhysicalDeviceFeatures(vkDevice.physicalDevice, &vkDevice.physicalDeviceFeatures); // load device features

	if (vkDevice.queueFamilyIndices.graphics != -1){
		vkGetDeviceQueue(vkDevice.logicalDevice, vkDevice.queueFamilyIndices.graphics, 0, &vkDevice.graphicsQueue);
	}
	if (vkDevice.queueFamilyIndices.present != -1){
		vkGetDeviceQueue(vkDevice.logicalDevice, vkDevice.queueFamilyIndices.present, 0, &vkDevice.presentQueue);
	}
	if (vkDevice.queueFamilyIndices.compute != -1){
		vkGetDeviceQueue(vkDevice.logicalDevice, vkDevice.queueFamilyIndices.compute, 0, &vkDevice.computeQueue);
	}
	if (vkDevice.queueFamilyIndices.transfer != -1){
		vkGetDeviceQueue(vkDevice.logicalDevice, vkDevice.queueFamilyIndices.transfer, 0, &vkDevice.transferQueue);
	}

	VkPipelineCacheCreateInfo cacheCreateInfo = {};
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheCreateInfo.flags = 0;
	cacheCreateInfo.initialDataSize = 0;
	vkCreatePipelineCache(vkDevice.logicalDevice, &cacheCreateInfo, vkCore.allocatorCallbacks, &vkDevice.pipelineCache);

	std::string selectedDeviceMessage("Selected ");
	selectedDeviceMessage += vkDevice.physicalDeviceProperties.deviceName;
	selectedDeviceMessage += " as render device";
	//LogCore::Log(selectedDeviceMessage, LOG_LEVEL_INFO); //TODO

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = vkDevice.physicalDevice;
	allocatorInfo.device = vkDevice.logicalDevice;

	vmaCreateAllocator(&allocatorInfo, &vkDevice.allocator);

	vulkan::InitializeVulkanDescriptorPool(vkDevice.logicalDevice, vkCore.allocatorCallbacks, vkDevice.descriptorPool);

	vkInitialized = true;
}

void VkCoreObjects::InitializeVulkanSwapchainForSDL()
{
	// Initialize swapchain of given window and return the created images
	std::vector<VkImage> swapchainImages =
	vulkan::InitializeVulkanSwapchainForSDL(
		vkDevice.physicalDevice,
		vkDevice.logicalDevice,
		vkCore.allocatorCallbacks,
		vkWindow.surface,
		vkWindow.window,
		vkWindow.swapchain,
		vkWindow.swapchainImageFormat,
		vkWindow.swapchainImageExtent
	);

	VulkanRenderPass newRenderPass = {};
	newRenderPass.clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f}; // black
	memset(&newRenderPass.clearValue, 0, sizeof(newRenderPass.clearValue));
	vulkan::InitializeVulkanRenderPass(vkDevice.logicalDevice, vkWindow.swapchainImageFormat, newRenderPass.renderPass);

	vkWindow.swapchainImageCount = static_cast<uint32_t>(swapchainImages.size());
	vkSwapchainImageStatuses.resize(vkWindow.swapchainImageCount);
	vkSwapchainImages.resize(vkWindow.swapchainImageCount);
	vkSwapchainCommandData.resize(vkWindow.swapchainImageCount);

	uint32_t frameIndex = 0;
	for (const VkImage& vkImage : swapchainImages)
	{
		vkSwapchainImages[frameIndex].image = vkImage;
		vkSwapchainImages[frameIndex].imageExtent = vkWindow.swapchainImageExtent;

		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = vkSwapchainImages[frameIndex].image;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = vkWindow.swapchainImageFormat;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(vkDevice.logicalDevice, &viewCreateInfo, nullptr, &vkSwapchainImages[frameIndex].imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image view!");
		}

		VkImageView attachments[] = {
			vkSwapchainImages[frameIndex].imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = newRenderPass.renderPass; // framebuffer render pass handle
		framebufferInfo.attachmentCount = 1; // how many attachments for this framebuffer
		framebufferInfo.pAttachments = attachments; // the attachments handle for this framebuffer
		framebufferInfo.width = vkWindow.swapchainImageExtent.width;
		framebufferInfo.height = vkWindow.swapchainImageExtent.height;
		framebufferInfo.layers = 1; // number of layers in the image array, 1 since each swap chain image is a single image

		if (vkCreateFramebuffer(vkDevice.logicalDevice, &framebufferInfo, nullptr, &vkSwapchainImages[frameIndex].framebuffer) != VK_SUCCESS) { // create the framebuffer object and store in the array we've created
			throw std::runtime_error("failed to create framebuffer!");
		}

		// create commandpool per swapchain image (Multi-threaded will need one pool PER thread, explicitly synced)
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = vkDevice.queueFamilyIndices.graphics;
		vkCreateCommandPool(vkDevice.logicalDevice, &poolInfo, vkCore.allocatorCallbacks, &vkSwapchainCommandData[frameIndex].commandPool);

		// create a primary commandbuffer per render target
		// We use these to record vulkan commands for the render target
		VkCommandBufferAllocateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferInfo.commandPool = vkSwapchainCommandData[frameIndex].commandPool;
		bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(vkDevice.logicalDevice, &bufferInfo, &vkSwapchainCommandData[frameIndex].primaryCommandBuffer);

		// Create a semaphore that signals that an image is ready to present to the swapchain
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(vkDevice.logicalDevice, &semaphoreInfo, vkCore.allocatorCallbacks, &vkSwapchainCommandData[frameIndex].taskCompleteSemaphore);

		vkCreateSemaphore(vkDevice.logicalDevice, &semaphoreInfo, vkCore.allocatorCallbacks, &vkSwapchainImageStatuses[frameIndex].imageAvailableSemaphore);

		frameIndex++;
	}

	vulkan::InitializeGraphicsPipeline(	vkDevice.logicalDevice,
										vkDevice.pipelineCache,
										vkCore.allocatorCallbacks,
										newRenderPass.renderPass,
										vkWindow.swapchainImageExtent,
										vkGraphicsPipeline.pipelineLayout,
										vkGraphicsPipeline.descriptorSetLayout,
										vkGraphicsPipeline.pipeline
									);
}

void VkCoreObjects::PrepareSwapchainFrame()
{
	VulkanSwapchainImageStatus& currentImage = vkSwapchainImageStatuses[vkWindow.currentFrameIndex];

	VkResult result = vkAcquireNextImageKHR(
		vkDevice.logicalDevice,
		vkWindow.swapchain,
		0,
		currentImage.imageAvailableSemaphore,
		VK_NULL_HANDLE,
		&currentImage.acquiredImageIndex
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { //FIXME Needed for Live Window Resizing
		//CleanupSwapchain();
		//CleanupTexturePipeline(); needed if extent changes
		//InitSwapChain();
		//InitRenderPass(); not needed unless format changes
		//InitFrameBuffers();
		//InitTexturePipeline();

		//return;
		result = vkAcquireNextImageKHR(
			vkDevice.logicalDevice,
			vkWindow.swapchain,
			0,
			currentImage.imageAvailableSemaphore,
			VK_NULL_HANDLE,
			&currentImage.acquiredImageIndex
		);
	}

	if (result == VK_SUCCESS){
		// Add current swapchain image tag to currentFrameIndex
		vkWindow.currentFrameIndex = (vkWindow.currentFrameIndex + 1) % vkWindow.swapchainImageCount;
	}
}

void VkCoreObjects::PresentSwapchainFrame()
{
	VulkanSwapchainImageStatus& imageToPresent = vkSwapchainImageStatuses[vkWindow.currentFrameIndex];
	VulkanTaskControl& swapchainImageControl = vkSwapchainCommandData[vkWindow.currentFrameIndex];

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &swapchainImageControl.taskCompleteSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &vkWindow.swapchain;
	presentInfo.pImageIndices = &imageToPresent.acquiredImageIndex;
	VkResult result = vkQueuePresentKHR(vkDevice.presentQueue, &presentInfo);
}

void VkCoreObjects::PrepareRenderTarget()
{
	VulkanTaskControl& controller = vkSwapchainCommandData[vkWindow.currentFrameIndex]; //TODO Multithread update
	VulkanImage& imageTarget = vkSwapchainImages[vkWindow.currentFrameIndex];

	vkResetCommandPool(vkDevice.logicalDevice, controller.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(vkSwapchainCommandData[vkWindow.currentFrameIndex].primaryCommandBuffer, &commandBufferBeginInfo);

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vkRenderPass.renderPass;
	renderPassBeginInfo.framebuffer = imageTarget.framebuffer; // destination of the renderpass
	renderPassBeginInfo.renderArea.extent.width = imageTarget.imageExtent.width;
	renderPassBeginInfo.renderArea.extent.height = imageTarget.imageExtent.height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &vkRenderPass.clearValue;
	vkCmdBeginRenderPass(controller.primaryCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

}

void VkCoreObjects::FinalizeRenderTarget()
{
	VulkanTaskControl& controller = vkSwapchainCommandData[vkWindow.currentFrameIndex]; //TODO Multithread update
	VulkanSwapchainImageStatus& swapchainImageStatus = vkSwapchainImageStatuses[vkWindow.currentFrameIndex];

	vkCmdEndRenderPass(controller.primaryCommandBuffer);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &controller.primaryCommandBuffer; // command buffer that holds all the commands we are submitting
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &controller.taskCompleteSemaphore; // Semaphores to be signaled upon completion of all command buffer tasks

	if (swapchainImageStatus.imageAvailableSemaphore != VK_NULL_HANDLE){
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &swapchainImageStatus.imageAvailableSemaphore;
	}

	vkEndCommandBuffer(controller.primaryCommandBuffer);
	vkQueueSubmit(vkDevice.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

void VkCoreObjects::TerminateVulkanImages()
{
	for (VulkanImage& image : vkImages){
		if (image.framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(vkDevice.logicalDevice, image.framebuffer, vkCore.allocatorCallbacks);
		if (image.imageView != VK_NULL_HANDLE)
			vkDestroyImageView(vkDevice.logicalDevice, image.imageView, vkCore.allocatorCallbacks);
		if (image.image != VK_NULL_HANDLE)
			vkDestroyImage(vkDevice.logicalDevice,image.image, vkCore.allocatorCallbacks);
	}
}

void VkCoreObjects::TerminateVulkanSwapchainImages()
{
	for (int index = 0; index < vkSwapchainImages.size(); ++index){
		if (vkSwapchainImages[index].imageView != VK_NULL_HANDLE){
			vkDestroyImageView(vkDevice.logicalDevice, vkSwapchainImages[index].imageView, vkCore.allocatorCallbacks);
			vkSwapchainImages[index].imageView = VK_NULL_HANDLE;
			vkSwapchainImages[index].image = VK_NULL_HANDLE; // Destroyed elsewhere in vkDestroySwapchainKHR
		}
		if (vkSwapchainImageStatuses[index].imageAvailableSemaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(vkDevice.logicalDevice, vkSwapchainImageStatuses[index].imageAvailableSemaphore, vkCore.allocatorCallbacks);
	}
}

void VkCoreObjects::TerminateVulkanGraphicsPipeline()
{
	if (vkGraphicsPipeline.descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(vkDevice.logicalDevice, vkGraphicsPipeline.descriptorSetLayout, vkCore.allocatorCallbacks);
	if (vkGraphicsPipeline.pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(vkDevice.logicalDevice, vkGraphicsPipeline.pipelineLayout, vkCore.allocatorCallbacks);
	if (vkGraphicsPipeline.pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(vkDevice.logicalDevice, vkGraphicsPipeline.pipeline, vkCore.allocatorCallbacks);
}

void VkCoreObjects::TerminateVulkanRenderPass()
{
	if (vkRenderPass.renderPass != VK_NULL_HANDLE)
		vkDestroyRenderPass(vkDevice.logicalDevice, vkRenderPass.renderPass, vkCore.allocatorCallbacks);
}

void VkCoreObjects::TerminateVulkanSwapchainForSDL()
{
	if (vkWindow.swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(vkDevice.logicalDevice, vkWindow.swapchain, vkCore.allocatorCallbacks);
	if (vkWindow.surface != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(vkCore.instance, vkWindow.surface, vkCore.allocatorCallbacks);
}

void VkCoreObjects::TerminateVulkanSwapchainTaskControls()
{
	for (VulkanTaskControl& taskData : vkSwapchainCommandData) {
			vkFreeCommandBuffers(vkDevice.logicalDevice, taskData.commandPool, 1, &taskData.primaryCommandBuffer);

		if (taskData.commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vkDevice.logicalDevice, taskData.commandPool, vkCore.allocatorCallbacks);
		if (taskData.taskCompleteSemaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(vkDevice.logicalDevice, taskData.taskCompleteSemaphore, vkCore.allocatorCallbacks);
	}
}

void VkCoreObjects::TerminateVulkanDevices()
{
	if (vkDevice.allocator != VK_NULL_HANDLE)
		vmaDestroyAllocator(vkDevice.allocator);
	if (vkDevice.pipelineCache != VK_NULL_HANDLE)
		vkDestroyPipelineCache(vkDevice.logicalDevice, vkDevice.pipelineCache, vkCore.allocatorCallbacks);
	if (vkDevice.descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(vkDevice.logicalDevice, vkDevice.descriptorPool, vkCore.allocatorCallbacks);
	if (vkDevice.logicalDevice != VK_NULL_HANDLE)
		vkDestroyDevice(vkDevice.logicalDevice, vkCore.allocatorCallbacks);
}

void VkCoreObjects::TerminateVulkanCore()
{
	vulkan::TerminateVulkanInstance(vkCore.instance, vkCore.allocatorCallbacks, vkCore.debugMessenger);
}

#endif
