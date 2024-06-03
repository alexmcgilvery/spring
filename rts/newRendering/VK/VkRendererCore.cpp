
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
	if (!vkInitialized_)
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

bool CVkRendererCore::InitializeVulkanForSDL(SDL_Window* _window_ptr)
{
	Vulkan::InitializeVulkanInstance(vkCore_.instance, vkCore_.allocator_ptr, vkCore_.message_back, Vulkan::GetRequiredSDLExtensions(_window_ptr));

	SDL_bool result = SDL_Vulkan_CreateSurface(_window_ptr, vkCore_.instance, &vkWindow_.display_surface);
	
	std::vector<VkPhysicalDevice> devices = Vulkan::DeterminePhysicalDevices(vkCore_.instance, Vulkan::RenderConfig::REQUIRED_DEVICE_EXTENSIONS);
	
	if (devices.size() == 0){
		vkInitialized_ = false;
		return vkInitialized_;
	}

	vkDevice_.physical_device = devices[0]; //TODO Better selection of device, IE when multiple devices supported

	Vulkan::InitializeVulkanDevice(vkDevice_.physical_device, vkDevice_.indices, vkDevice_.logical_device, vkWindow_.display_surface,Vulkan::RenderConfig::REQUIRED_DEVICE_EXTENSIONS);

	vkGetPhysicalDeviceProperties(vkDevice_.physical_device, &vkDevice_.physical_device_properties);
	vkGetPhysicalDeviceFeatures(vkDevice_.physical_device, &vkDevice_.physical_device_features);

	//TODO Better Queue selection/handling
	if (vkDevice_.indices.graphics != -1){
		vkGetDeviceQueue(vkDevice_.logical_device, vkDevice_.indices.graphics, 0, &vkDevice_.graphics_queue); 
	}
	if (vkDevice_.indices.present != -1){
		vkGetDeviceQueue(vkDevice_.logical_device, vkDevice_.indices.present, 0, &vkDevice_.present_queue); 
	}
	if (vkDevice_.indices.compute != -1){
		vkGetDeviceQueue(vkDevice_.logical_device, vkDevice_.indices.compute, 0, &vkDevice_.compute_queue); 
	}
	if (vkDevice_.indices.transfer != -1){
		vkGetDeviceQueue(vkDevice_.logical_device, vkDevice_.indices.transfer, 0, &vkDevice_.transfer_queue); 
	}

	VkPipelineCacheCreateInfo cache_create_info = {};
	cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cache_create_info.flags = 0;
	cache_create_info.initialDataSize = 0;
	vkCreatePipelineCache(vkDevice_.logical_device, &cache_create_info, vkCore_.allocator_ptr, &vkDevice_.pipeline_cache);

	std::string output("Selected ");
	output += vkDevice_.physical_device_properties.deviceName;
	output += " as render device";
	//LogCore::Log(output, LOG_LEVEL_INFO); //TODO

	VmaAllocatorCreateInfo allocator_info = {};
	allocator_info.physicalDevice = vkDevice_.physical_device;
	allocator_info.device = vkDevice_.logical_device;

	vmaCreateAllocator(&allocator_info, &vkDevice_.allocator);

	Vulkan::InitializeVulkanDescriptorPool(vkDevice_.logical_device, vkCore_.allocator_ptr, vkDevice_.descriptor_pool);

	vkInitialized_ = true;
	return vkInitialized_;
}

void CVkRendererCore::InitializeVulkanSwapchainForSDL()
{
	// Initialize swapchain of given window and return the created images
	std::vector<VkImage> swapchain_images = 
	Vulkan::InitializeVulkanSwapChainForSDL(
		vkDevice_.physical_device, 
		vkDevice_.logical_device,
		vkCore_.allocator_ptr,
		vkWindow_.display_surface, 
		vkWindow_.window_ptr, 
		vkWindow_.swapchain, 
		vkWindow_.swapchain_image_format, 
		vkWindow_.swapchain_image_extent
	);

	VulkanRenderPass new_render_pass = {};
	new_render_pass.clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f}; // black 
	memset(&new_render_pass.clear_value, 0, sizeof(new_render_pass.clear_value));
	Vulkan::InitializeVulkanRenderPass(vkDevice_.logical_device, vkWindow_.swapchain_image_format, new_render_pass.render_pass);
	
	vkWindow_.swapchain_image_count = static_cast<uint32_t>(swapchain_images.size());
	vkSwapchainImageStatuses_.resize(vkWindow_.swapchain_image_count);
	vkSwapchainImages_.resize(vkWindow_.swapchain_image_count);
	vkSwapchainCommandData_.resize(vkWindow_.swapchain_image_count);

	uint32_t frame_index = 0;
	for (const VkImage& vk_image : swapchain_images)
	{
		vkSwapchainImages_[frame_index].image = vk_image;
		vkSwapchainImages_[frame_index].image_extent = vkWindow_.swapchain_image_extent;

		VkImageViewCreateInfo view_create_info = {};
		view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_create_info.image = vkSwapchainImages_[frame_index].image;
		view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_create_info.format = vkWindow_.swapchain_image_format;
		view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_create_info.subresourceRange.baseMipLevel = 0;
		view_create_info.subresourceRange.levelCount = 1;
		view_create_info.subresourceRange.baseArrayLayer = 0;
		view_create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(vkDevice_.logical_device, &view_create_info, nullptr, &vkSwapchainImages_[frame_index].image_view) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image view!");
		}

		VkImageView attachments[] = {
			vkSwapchainImages_[frame_index].image_view
		};

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = new_render_pass.render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = vkWindow_.swapchain_image_extent.width;
		framebuffer_info.height = vkWindow_.swapchain_image_extent.height;
		framebuffer_info.layers = 1; // number of layers in the image array, 1 since each swap chain image is a single image

		if (vkCreateFramebuffer(vkDevice_.logical_device, &framebuffer_info, nullptr, &vkSwapchainImages_[frame_index].framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}

		// create commandpool per swapchain image (Multi-threaded will need one pool PER thread, explicitly synced)
		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = vkDevice_.indices.graphics;
		vkCreateCommandPool(vkDevice_.logical_device, &pool_info, vkCore_.allocator_ptr, &vkSwapchainCommandData_[frame_index].command_pool);
		
		// create a primary commandbuffer per render target
		// We use these to record vulkan commands for the render target
		VkCommandBufferAllocateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		buffer_info.commandPool = vkSwapchainCommandData_[frame_index].command_pool;
		buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		buffer_info.commandBufferCount = 1;
		vkAllocateCommandBuffers(vkDevice_.logical_device, &buffer_info, &vkSwapchainCommandData_[frame_index].primary_buffer);

		// Create a semaphore that signals that an image is ready to present to the swapchain
		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(vkDevice_.logical_device, &semaphore_info, vkCore_.allocator_ptr, &vkSwapchainCommandData_[frame_index].task_complete_semaphore);

		vkCreateSemaphore(vkDevice_.logical_device, &semaphore_info, vkCore_.allocator_ptr, &vkSwapchainImageStatuses_[frame_index].image_available_semaphore);

		frame_index++;
	}

	Vulkan::InitializeGraphicsPipeline(	
		vkDevice_.logical_device, 
		vkDevice_.pipeline_cache, 
		vkCore_.allocator_ptr, 
		new_render_pass.render_pass, 
		vkWindow_.swapchain_image_extent, 
		vkGraphicsPipeline_.pipeline_layout, 
		vkGraphicsPipeline_.descriptor_set_layout, 
		vkGraphicsPipeline_.pipeline
	);
}

void CVkRendererCore::PrepareSwapchainFrame()
{
	VulkanSwapchainImageStatus& current_image = vkSwapchainImageStatuses_[vkWindow_.current_frame_index];

	VkResult result = vkAcquireNextImageKHR(
		vkDevice_.logical_device, 
		vkWindow_.swapchain, 
		0, 
		current_image.image_available_semaphore, 
		VK_NULL_HANDLE, 
		&current_image.frame_index
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
			vkDevice_.logical_device, 
			vkWindow_.swapchain, 
			0, 
			current_image.image_available_semaphore, 
			VK_NULL_HANDLE, 
			&current_image.frame_index
		);
	}

	if (result == VK_SUCCESS){
		// Add current swapchain image tag to current_frame_index
		vkWindow_.current_frame_index = (vkWindow_.current_frame_index + 1) % vkWindow_.swapchain_image_count;
	}
}

void CVkRendererCore::PresentSwapchainFrame()
{
	VulkanSwapchainImageStatus& image_to_present = vkSwapchainImageStatuses_[vkWindow_.current_frame_index];
	VulkanTaskControl& swapchain_image_control = vkSwapchainCommandData_[vkWindow_.current_frame_index];

	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &swapchain_image_control.task_complete_semaphore;
	info.swapchainCount = 1;
	info.pSwapchains = &vkWindow_.swapchain;
	info.pImageIndices = &image_to_present.frame_index;
	VkResult result = vkQueuePresentKHR(vkDevice_.present_queue, &info);
}

void CVkRendererCore::PrepareRenderTarget()
{
	VulkanTaskControl& controller = vkSwapchainCommandData_[vkWindow_.current_frame_index]; //TODO Multithread update
	VulkanImage& image_target = vkSwapchainImages_[vkWindow_.current_frame_index];

	vkResetCommandPool(vkDevice_.logical_device, controller.command_pool, 0); 

	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(vkSwapchainCommandData_[vkWindow_.current_frame_index].primary_buffer, &command_buffer_begin_info);

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = vkRenderPass_.render_pass;
	render_pass_begin_info.framebuffer = image_target.framebuffer; // destination of the renderpass
	render_pass_begin_info.renderArea.extent.width = image_target.image_extent.width;
	render_pass_begin_info.renderArea.extent.height = image_target.image_extent.height;
	render_pass_begin_info.clearValueCount = 1;
	render_pass_begin_info.pClearValues = &vkRenderPass_.clear_value;
	vkCmdBeginRenderPass(controller.primary_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void CVkRendererCore::FinalizeRenderTarget()
{
	VulkanTaskControl& controller = vkSwapchainCommandData_[vkWindow_.current_frame_index]; //TODO Multithread update
	VulkanSwapchainImageStatus& swapchain_image_status = vkSwapchainImageStatuses_[vkWindow_.current_frame_index];
	
	vkCmdEndRenderPass(controller.primary_buffer); 

	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = &wait_stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &controller.primary_buffer; // command buffer that holds all the commands we are submitting
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &controller.task_complete_semaphore; // Semaphores to be signaled upon completion of all command buffer tasks
	
	if (swapchain_image_status.image_available_semaphore != VK_NULL_HANDLE){
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &swapchain_image_status.image_available_semaphore;
	}

	vkEndCommandBuffer(controller.primary_buffer);
	vkQueueSubmit(vkDevice_.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
}

void CVkRendererCore::TerminateVulkanImages()
{
	for (VulkanImage& image : vkImages_){
		if (image.framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(vkDevice_.logical_device, image.framebuffer, vkCore_.allocator_ptr);
		if (image.image_view != VK_NULL_HANDLE)
			vkDestroyImageView(vkDevice_.logical_device, image.image_view, vkCore_.allocator_ptr);
		if (image.image != VK_NULL_HANDLE)
			vkDestroyImage(vkDevice_.logical_device,image.image, vkCore_.allocator_ptr);
	}
}

void CVkRendererCore::TerminateVulkanSwapchainImages()
{
	for (int index = 0; index < vkSwapchainImages_.size(); ++index){
		if (vkSwapchainImages_[index].image_view != VK_NULL_HANDLE){
			vkDestroyImageView(vkDevice_.logical_device, vkSwapchainImages_[index].image_view, vkCore_.allocator_ptr);
			vkSwapchainImages_[index].image_view = VK_NULL_HANDLE;
			vkSwapchainImages_[index].image = VK_NULL_HANDLE; // Destroyed elsewhere in vkDestroySwapchainKHR
		}
		if (vkSwapchainImageStatuses_[index].image_available_semaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(vkDevice_.logical_device, vkSwapchainImageStatuses_[index].image_available_semaphore, vkCore_.allocator_ptr);
	}
}

void CVkRendererCore::TerminateVulkanGraphicsPipeline()
{
	if (vkGraphicsPipeline_.descriptor_set_layout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(vkDevice_.logical_device, vkGraphicsPipeline_.descriptor_set_layout, vkCore_.allocator_ptr);
	if (vkGraphicsPipeline_.pipeline_layout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(vkDevice_.logical_device, vkGraphicsPipeline_.pipeline_layout, vkCore_.allocator_ptr);
	if (vkGraphicsPipeline_.pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(vkDevice_.logical_device, vkGraphicsPipeline_.pipeline, vkCore_.allocator_ptr);
}

void CVkRendererCore::TerminateVulkanRenderPass()
{
	if (vkRenderPass_.render_pass != VK_NULL_HANDLE)
		vkDestroyRenderPass(vkDevice_.logical_device, vkRenderPass_.render_pass, vkCore_.allocator_ptr);
}

void CVkRendererCore::TerminateVulkanSwapchainForSDL()
{
	if (vkWindow_.swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(vkDevice_.logical_device, vkWindow_.swapchain, vkCore_.allocator_ptr);
	if (vkWindow_.display_surface != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(vkCore_.instance, vkWindow_.display_surface, vkCore_.allocator_ptr);
}

void CVkRendererCore::TerminateVulkanSwapchainTaskControls()
{
	for (VulkanTaskControl& taskData : vkSwapchainCommandData_) {
			vkFreeCommandBuffers(vkDevice_.logical_device, taskData.command_pool, 1, &taskData.primary_buffer);

		if (taskData.command_pool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vkDevice_.logical_device, taskData.command_pool, vkCore_.allocator_ptr);
		if (taskData.task_complete_semaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(vkDevice_.logical_device, taskData.task_complete_semaphore, vkCore_.allocator_ptr);
	}
}

void CVkRendererCore::TerminateVulkanDevices()
{
	if (vkDevice_.allocator != VK_NULL_HANDLE)
		vmaDestroyAllocator(vkDevice_.allocator);
	if (vkDevice_.pipeline_cache != VK_NULL_HANDLE)
		vkDestroyPipelineCache(vkDevice_.logical_device, vkDevice_.pipeline_cache, vkCore_.allocator_ptr);
	if (vkDevice_.descriptor_pool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(vkDevice_.logical_device, vkDevice_.descriptor_pool, vkCore_.allocator_ptr);
	if (vkDevice_.logical_device != VK_NULL_HANDLE)
		vkDestroyDevice(vkDevice_.logical_device, vkCore_.allocator_ptr);
}

void CVkRendererCore::TerminateVulkanCore()
{
	Vulkan::TerminateVulkanInstance(vkCore_.instance, vkCore_.allocator_ptr, vkCore_.message_back);
}