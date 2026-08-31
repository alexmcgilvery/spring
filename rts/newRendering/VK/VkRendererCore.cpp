
#include "VkRendererCore.h"
#include "VkFuncs.h"
#include "VkConfig.h"
#include "newRendering/GlobalRendering.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/WindowManagerHelper.h"

#include <SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

CVkRendererCore* vkRenderer;

CVkRendererCore::CVkRendererCore()
{
	// Not needed
}

CVkRendererCore::~CVkRendererCore()
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

void CVkRendererCore::RendererPreWindowInit()
{
	// Not needed for vulkan?
}

void CVkRendererCore::RendererPostWindowInit()
{ //TODO
	InitializeVulkanForSDL(sdlWindow);
}

bool CVkRendererCore::RendererCreateWindow(const char *title)
{ //TODO
	return false;
}

void CVkRendererCore::RendererSetStartState()
{ //TODO
	LOG("[GR::%s]", __func__);

	//vkCoreObject.InitializeVulkanForSDL(sdlWindow);
	//vkCoreObject.InitializeVulkanSwapchainForSDL();
}

SDL_Window* CVkRendererCore::RendererCreateSDLWindow(const char* title)
{
	SDL_Window* newWindow = nullptr;

	const char* frmts[2] = {
		"[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer for main window",
		"[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\") for main window",
	};

	bool borderless_ = configHandler->GetBool("WindowBorderless");
	bool fullScreen_ = configHandler->GetBool("Fullscreen");
	int winPosX_ = configHandler->GetInt("WindowPosX");
	int winPosY_ = configHandler->GetInt("WindowPosY");
	int2 newRes = GetCfgWinRes();

	uint32_t 	sdlFlags  = (borderless_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) * fullScreen_;
				sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless_);
				sdlFlags |= (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_Vulkan_LoadLibrary(nullptr);
	
	if ((newWindow = SDL_CreateWindow(title, winPosX_, winPosY_, newRes.x, newRes.y, sdlFlags)) == nullptr ){
		LOG_L(L_WARNING, frmts[0], __func__ ,SDL_GetError());
	};
}

void CVkRendererCore::RendererDestroyWindow()
{ //TODO

}

void CVkRendererCore::RendererUpdateWindow()
{ //TODO

}

void CVkRendererCore::RendererPresentFrame(bool allowSwapBuffers, bool clearErrors)
{ //TODO
	spring_time pre;
	{
		SCOPED_TIMER("Misc::SwapBuffers");
		assert(sdlWindow);

		// silently or verbosely clear queue at the end of every frame
		if (clearErrors || rendererDebugErrors)
			//glClearErrors("GR", __func__, rendererDebugErrors);

		if (!allowSwapBuffers && !forceSwapBuffers)
			return;

		pre = spring_now();

		//RenderBuffer::SwapRenderBuffers(); //all RBs are swapped here
		//IStreamBufferConcept::PutBufferLocks();

		//https://stackoverflow.com/questions/68480028/supporting-opengl-screen-capture-by-third-party-applications
		//glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);

		//SDL_GL_SwapWindow(sdlWindow);
		FrameMark;
	}
	// exclude debug from SCOPED_TIMER("Misc::SwapBuffers");
	eventHandler.DbgTimingInfo(TIMING_SWAP, pre, spring_now());
	lastSwapBuffersEnd = spring_now();
}

void CVkRendererCore::UpdateViewport()
{ //TODO

}

void CVkRendererCore::SetTimeStamp(uint32_t queryIdx) const
{ //TODO

}

uint64_t CVkRendererCore::CalculateFrameTimeDelta(uint32_t queryIdx0, uint32_t queryIdx1) const
{ //TODO

}

void CVkRendererCore::ToggleMultisampling() const
{ //TODO

}

bool CVkRendererCore::ToggleDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const
{ //TODO

}

void CVkRendererCore::AquireThreadContext()
{
	//Hack not needed for Vulkan
}

void CVkRendererCore::ReleaseThreadContext()
{
	//Hack not needed for Vulkan
}

void CVkRendererCore::InitStatic()
{
	alignas(CVkRendererCore) static std::byte globalRenderingMem[sizeof(CVkRendererCore)];
	vkRenderer = new (globalRenderingMem) CVkRendererCore();
	globalRendering = (CGlobalRendering*) vkRenderer;
}

void CVkRendererCore::KillStatic()
{
	globalRendering->PreKill();
	spring::SafeDestruct(vkRenderer);
}

bool CVkRendererCore::InitializeVulkanForSDL(SDL_Window* window)
{
	vulkan::InitializeVulkanInstance(vkCore.instance, vkCore.allocatorCallbacks, vkCore.debugMessenger, vulkan::GetRequiredSDLExtensions(window));

	SDL_bool result = SDL_Vulkan_CreateSurface(window, vkCore.instance, &vkWindow.surface);

	std::vector<VkPhysicalDevice> devices = vulkan::DeterminePhysicalDevices(vkCore.instance, vulkan::render_config::REQUIRED_DEVICE_EXTENSIONS);

	if (devices.size() == 0){
		vkInitialized = false;
		return vkInitialized;
	}

	vkDevice.physicalDevice = devices[0]; //TODO Better selection of device, IE when multiple devices supported

	vulkan::InitializeVulkanDevice(vkDevice.physicalDevice, vkDevice.queueFamilyIndices, vkDevice.logicalDevice, vkWindow.surface,vulkan::render_config::REQUIRED_DEVICE_EXTENSIONS);

	vkGetPhysicalDeviceProperties(vkDevice.physicalDevice, &vkDevice.physicalDeviceProperties);
	vkGetPhysicalDeviceFeatures(vkDevice.physicalDevice, &vkDevice.physicalDeviceFeatures);

	//TODO Better Queue selection/handling
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
	return vkInitialized;
}

void CVkRendererCore::InitializeVulkanSwapchainForSDL()
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
		framebufferInfo.renderPass = newRenderPass.renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = vkWindow.swapchainImageExtent.width;
		framebufferInfo.height = vkWindow.swapchainImageExtent.height;
		framebufferInfo.layers = 1; // number of layers in the image array, 1 since each swap chain image is a single image

		if (vkCreateFramebuffer(vkDevice.logicalDevice, &framebufferInfo, nullptr, &vkSwapchainImages[frameIndex].framebuffer) != VK_SUCCESS) {
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

	vulkan::InitializeGraphicsPipeline(
		vkDevice.logicalDevice,
		vkDevice.pipelineCache,
		vkCore.allocatorCallbacks,
		newRenderPass.renderPass,
		vkWindow.swapchainImageExtent,
		vkGraphicsPipeline.pipelineLayout,
		vkGraphicsPipeline.descriptorSetLayout,
		vkGraphicsPipeline.pipeline
	);
}

void CVkRendererCore::PrepareSwapchainFrame()
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

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { //FIXME Needed for Window Resizing
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

void CVkRendererCore::PresentSwapchainFrame()
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

void CVkRendererCore::PrepareRenderTarget()
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

void CVkRendererCore::FinalizeRenderTarget()
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

void CVkRendererCore::TerminateVulkanImages()
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

void CVkRendererCore::TerminateVulkanSwapchainImages()
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

void CVkRendererCore::TerminateVulkanGraphicsPipeline()
{
	if (vkGraphicsPipeline.descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(vkDevice.logicalDevice, vkGraphicsPipeline.descriptorSetLayout, vkCore.allocatorCallbacks);
	if (vkGraphicsPipeline.pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(vkDevice.logicalDevice, vkGraphicsPipeline.pipelineLayout, vkCore.allocatorCallbacks);
	if (vkGraphicsPipeline.pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(vkDevice.logicalDevice, vkGraphicsPipeline.pipeline, vkCore.allocatorCallbacks);
}

void CVkRendererCore::TerminateVulkanRenderPass()
{
	if (vkRenderPass.renderPass != VK_NULL_HANDLE)
		vkDestroyRenderPass(vkDevice.logicalDevice, vkRenderPass.renderPass, vkCore.allocatorCallbacks);
}

void CVkRendererCore::TerminateVulkanSwapchainForSDL()
{
	if (vkWindow.swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(vkDevice.logicalDevice, vkWindow.swapchain, vkCore.allocatorCallbacks);
	if (vkWindow.surface != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(vkCore.instance, vkWindow.surface, vkCore.allocatorCallbacks);
}

void CVkRendererCore::TerminateVulkanSwapchainTaskControls()
{
	for (VulkanTaskControl& taskData : vkSwapchainCommandData) {
			vkFreeCommandBuffers(vkDevice.logicalDevice, taskData.commandPool, 1, &taskData.primaryCommandBuffer);

		if (taskData.commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vkDevice.logicalDevice, taskData.commandPool, vkCore.allocatorCallbacks);
		if (taskData.taskCompleteSemaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(vkDevice.logicalDevice, taskData.taskCompleteSemaphore, vkCore.allocatorCallbacks);
	}
}

void CVkRendererCore::TerminateVulkanDevices()
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

void CVkRendererCore::TerminateVulkanCore()
{
	vulkan::TerminateVulkanInstance(vkCore.instance, vkCore.allocatorCallbacks, vkCore.debugMessenger);
}
