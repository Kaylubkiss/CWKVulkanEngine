#include "vkContextBase.h"
#include "vkUtility.h"
#include "vkInit.h"

namespace vk
{	

	//constructor
	ContextBase::ContextBase()
	{
		assert(_Application != NULL);

		CreateWindow();
		CreateInstance();

		if (SDL_Vulkan_CreateSurface(window.sdl_ptr, instance, &window.surface) != SDL_TRUE)
		{
			throw std::runtime_error("could not create window surface! " + std::string(SDL_GetError()));
		}
		
		device.Initialize(instance, window.surface);
		window.contextPhysicalDevice = device.physical;

		std::array<uint32_t, 2> queueFamilies = { device.graphicsQueue.family, device.presentQueue.family };
		swapChain.Init(&this->device, window); //need window for its surface and viewport info.
		
		swapChain.Create(window);

		//conforms to higher frame counts to prevent flickering.
		if (swapChain.createInfo.minImageCount > settings.maxFramesInFlight)
		{
			settings.maxFramesInFlight = swapChain.createInfo.minImageCount;
		}

		InitializeRenderPass();
		this->swapChain.CreateFrameBuffers(window.viewport, renderPass);

		CreateSynchronizationPrimitives();

		this->commandPool = vk::init::CommandPool(device.logical, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		//each swapchain should have its own command buffer
		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = vk::init::CommandBufferAllocateInfo();
		cmdBufferAllocateInfo.commandBufferCount = (uint32_t)this->commandBuffers.size();
		cmdBufferAllocateInfo.commandPool = this->commandPool;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device.logical, &cmdBufferAllocateInfo, commandBuffers.data()));

		if (settings.UIEnabled) 
		{
			UserInterfaceInitInfo userInterfaceCI = {};
			userInterfaceCI.contextInstance = this->instance;
			userInterfaceCI.contextLogicalDevice = this->device.logical;
			userInterfaceCI.contextPhysicalDevice = this->device.physical;
			userInterfaceCI.contextQueue = this->device.graphicsQueue;
			userInterfaceCI.contextWindow = this->window.sdl_ptr;
			userInterfaceCI.renderPass = this->renderPass;
			userInterfaceCI.minImages = settings.maxFramesInFlight;

			this->UIOverlay = UserInterface(userInterfaceCI);
		}

		ContextBase::FillOutGraphicsContextInfo();

		this->mCamera = Camera({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f }, { 0,1,0 });

		this->pipelineManager.Init(mInfo);

	}

	//destructor
	ContextBase::~ContextBase()
	{
		pipelineManager.Destroy();
		swapChain.Destroy();
		UIOverlay.Destroy();

		vkDestroyPipelineLayout(device.logical, pipelineLayout, nullptr);
		vkDestroyDescriptorPool(device.logical, this->descriptorPool, nullptr);

		vkFreeCommandBuffers(device.logical, this->commandPool, this->commandBuffers.size(), this->commandBuffers.data());
		vkDestroyCommandPool(device.logical, this->commandPool, nullptr);

		//semaphores
		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			vkDestroySemaphore(this->device.logical, presentCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(this->device.logical, renderCompleteSemaphores[i], nullptr);

			vkDestroyFence(device.logical, inFlightFences[i], nullptr);
		}

		device.Destroy();
		
		vkDestroySurfaceKHR(this->instance, this->window.surface, nullptr);
		vkDestroyInstance(this->instance, nullptr);
	}

	//helper(s)
	void ContextBase::CreateInstance()
	{
		assert(window.sdl_ptr != nullptr);

		//create instance info.
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_4;
		appInfo.pApplicationName = "Caleb Vulkan Engine";
		appInfo.engineVersion = 1;
		appInfo.pNext = nullptr;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.flags = 0;

		createInfo.pApplicationInfo = &appInfo;

		unsigned int sdl_extensionCount = 0;
		if (SDL_Vulkan_GetInstanceExtensions(window.sdl_ptr, &sdl_extensionCount, nullptr) != SDL_TRUE)
		{
			throw std::runtime_error("could not grab extensions from SDL!");
		}

		std::vector<const char*> extensionNames(sdl_extensionCount);

		if (SDL_Vulkan_GetInstanceExtensions(window.sdl_ptr, &sdl_extensionCount, extensionNames.data()) != SDL_TRUE)
		{
			throw std::runtime_error("could not grab extensions from SDL!");
		}

		createInfo.enabledExtensionCount = extensionNames.size();
		createInfo.ppEnabledExtensionNames = extensionNames.data();


		//enabling validation layers
 		const char* layerName = "VK_LAYER_KHRONOS_validation";

		const VkBool32 setting_validate_core = VK_TRUE;
		const VkBool32 setting_validate_sync = VK_TRUE;
		const VkBool32 setting_thread_safety = VK_TRUE;
		const char* setting_debug_action[] = { "VK_DBG_LAYER_ACTION_LOG_MSG" };
		const char* setting_report_flags[] = { "error"  };
		const VkBool32 setting_enable_message_limit = VK_TRUE;
		const int32_t setting_duplicate_message_limit = 3;

		const VkLayerSettingEXT settings[] = {
			{layerName, "validate_core", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_core},
			{layerName, "duplicate_message_limit", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &setting_duplicate_message_limit},
			{layerName, "report_flags", VK_LAYER_SETTING_TYPE_STRING_EXT, static_cast<uint32_t>(std::size(setting_report_flags)), setting_report_flags},
			/*{layerName, "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_sync},
			{layerName, "thread_safety", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_thread_safety},
			{layerName, "debug_action", VK_LAYER_SETTING_TYPE_STRING_EXT, 1, setting_debug_action},
			{layerName, "report_flags", VK_LAYER_SETTING_TYPE_STRING_EXT, static_cast<uint32_t>(std::size(setting_report_flags)), setting_report_flags},
			{layerName, "enable_message_limit", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_enable_message_limit},
			{layerName, "duplicate_message_limit", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &setting_duplicate_message_limit} */
		};

		const VkLayerSettingsCreateInfoEXT layer_settings_create_info = {
			VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr,
			static_cast<uint32_t>(std::size(settings)), settings };


		createInfo.pNext = &layer_settings_create_info;

		std::array<const char*, 1> instanceLayers =
		{
			layerName
		};

		if (!vk::util::CheckLayerSupport(instanceLayers.data(), instanceLayers.size()))
		{
			throw std::runtime_error("one or more layers are not supported\n");
		}

		createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
		createInfo.ppEnabledLayerNames = instanceLayers.data();
	
		//create instance.
		//this function, if successful, will create a "handle object"
		//and make pInstance the handle. A handle is always 64-bits wide.  

		//also, setting the pAllocator to null will make vulkan do its
		//own memory management, whereas we can create our own allocator
		//for vulkan to use

		VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &this->instance))


	}

	void ContextBase::CreateWindow() 
	{
		window.viewport.width = 640;
		window.viewport.height = 480;
		window.viewport.minDepth = 0;
		window.viewport.maxDepth = 1;

		window.scissor.extent.width = (uint32_t)window.viewport.width;
		window.scissor.extent.height = (uint32_t)window.viewport.height;

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		}

		window.sdl_ptr = SDL_CreateWindow("Caleb's Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			int(window.viewport.width), int(window.viewport.height), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS);

		if (window.sdl_ptr == nullptr)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}

		SDL_RaiseWindow(window.sdl_ptr);

		window.isPrepared = true;
	}

	void ContextBase::CreateSynchronizationPrimitives() 
	{
		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			inFlightFences[i] = vk::init::CreateFence(device.logical);
			presentCompleteSemaphores[i] = vk::init::CreateSemaphore(this->device.logical);
			renderCompleteSemaphores[i] = vk::init::CreateSemaphore(this->device.logical);
		}
	}
	
	void ContextBase::UpdateUI() {}

	void ContextBase::ResizeWindowDerived() {}

	void ContextBase::ResizeWindow() 
	{
		//std::cout << "resizing window\n";

		if (window.isMinimized)
		{
			window.isPrepared = false;
			return;
		}

		VK_CHECK_RESULT(vkDeviceWaitIdle(this->device.logical));
		

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->device.physical, window.surface, &surfaceCapabilities));

		window.UpdateExtents(surfaceCapabilities.currentExtent);

		swapChain.Recreate(renderPass, window);

		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			vkDestroySemaphore(this->device.logical, presentCompleteSemaphores[i], nullptr);
			presentCompleteSemaphores[i] = VK_NULL_HANDLE;

			vkDestroySemaphore(this->device.logical, renderCompleteSemaphores[i], nullptr);
			renderCompleteSemaphores[i] = VK_NULL_HANDLE;

			vkDestroyFence(device.logical, inFlightFences[i], nullptr);
			inFlightFences[i] = VK_NULL_HANDLE;
		}

		CreateSynchronizationPrimitives();

		ResizeWindowDerived();

		window.isPrepared = true;
		
	}

	//initializers

	void ContextBase::InitializeRenderPass() 
	{
		assert(swapChain.handle != VK_NULL_HANDLE);

		std::array<VkAttachmentDescription, 2> attachments = {};

		//color attachment
		attachments[0].format = swapChain.createInfo.imageFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		//depth attachment
		attachments[1].format = swapChain.depthAttachment.format;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		//for layout transitions
		std::array<VkSubpassDependency, 2> dependencies{};

		//depth writing/reading dependencies
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = dependencies[0].srcStageMask;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;

		//color writing/reading dependencies. This is to ensure that the color attachment read/writes are finished before subpass 0 begins and uses them again for reading/writing.
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = dependencies[1].srcStageMask;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //this can also be 0
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[1].dependencyFlags = 0;

		VkRenderPassCreateInfo renderPassCI = {};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = attachments.size();
		renderPassCI.pAttachments = attachments.data();
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpassDescription;
		renderPassCI.dependencyCount = dependencies.size();
		renderPassCI.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device.logical, &renderPassCI, nullptr, &renderPass));
	}

	void ContextBase::FillOutGraphicsContextInfo() 
	{
		//TODO: a little janky way to initialize as more of mInfo is filled with derived classes.
		mInfo.logicalDevice = device.logical;
		mInfo.physicalDevice = device.physical;
		mInfo.graphicsQueue = device.graphicsQueue;

		if (settings.UIEnabled) 
		{
			mInfo.contextUIPtr = &UIOverlay;
		}
	}

	//getter(s)
	const VkPhysicalDevice ContextBase::PhysicalDevice() const 
	{
		return this->device.physical;
	}

	const VkDevice ContextBase::LogicalDevice() const 
	{
		return this->device.logical;
	}

	Camera& ContextBase::GetCamera()
	{
		return this->mCamera;
	}

	vk::Window& ContextBase::GetWindow() 
	{
		return this->window;
	}

	GraphicsContextInfo ContextBase::GetGraphicsContextInfo() 
	{
		return mInfo;
	}

	void ContextBase::WaitForDevice()
	{
		if (this->device.logical)
		{
			VK_CHECK_RESULT(vkDeviceWaitIdle(this->device.logical));
		}
	}

	void ContextBase::PrepareFrame() 
	{
		//add synchronization calls here.
		VK_CHECK_RESULT(vkWaitForFences(device.logical, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device.logical, 1, &inFlightFences[currentFrame]));

		if (settings.UIEnabled) 
		{
			UIOverlay.Prepare();
			if (!ImGui::Begin("CWKVulkanEngine"))
			{
				ImGui::End();
			}
			else
			{
				UpdateUI();
				ImGui::End();
			}
		}

		VkResult result = vkAcquireNextImageKHR(device.logical, swapChain.handle, UINT64_MAX, presentCompleteSemaphores[currentFrame], (VkFence)nullptr, &currentImageIndex);
	
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR) 
			{
				ResizeWindow();
			}
			return;
		}
		else
		{
			VK_CHECK_RESULT(result);
		}

	}

	void ContextBase::SubmitFrame() 
	{
		VkSubmitInfo submitInfo = {};
		const VkPipelineStageFlags pipelineWaitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
		submitInfo.pWaitDstStageMask = &pipelineWaitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentImageIndex];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &this->commandBuffers[currentFrame];
		VK_CHECK_RESULT(vkQueueSubmit(this->device.graphicsQueue.handle, 1, &submitInfo, inFlightFences[currentFrame]))

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentImageIndex];
		presentInfo.pImageIndices = &currentImageIndex;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &this->swapChain.handle;
		VkResult result = vkQueuePresentKHR(this->device.presentQueue.handle, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR) 
			{
				ResizeWindow();
			}
			return;
		}
		else
		{
			VK_CHECK_RESULT(result);
		}

		currentFrame = (currentFrame + 1) % settings.maxFramesInFlight;

	}

	void ContextBase::ToggleRendering()
	{
		window.isPrepared = !window.isPrepared;
	}
}