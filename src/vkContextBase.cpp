#include "vkContextBase.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "ApplicationGlobal.h"
#include "ObjectManager.h"
#include "HotReloader.h"
#include <SDL2/SDL_vulkan.h>

namespace vk
{	
	//destructor
	ContextBase::~ContextBase()
	{
		mPipeline.Destroy(this->device.logical);
		swapChain.Destroy();
		uTransform.buffer.Destroy();

		depthStencil.Destroy(device.logical);

		vkDestroyDescriptorPool(device.logical, this->descriptorPool, nullptr);

		vkFreeCommandBuffers(device.logical, this->commandPool, this->commandBuffers.size(), this->commandBuffers.data());
		vkDestroyCommandPool(device.logical, this->commandPool, nullptr);

		//semaphores
		vkDestroySemaphore(this->device.logical, semaphores.presentComplete, nullptr);
		vkDestroySemaphore(this->device.logical, semaphores.renderComplete, nullptr);

		vkDestroyDevice(this->device.logical, nullptr);
		
		vkDestroySurfaceKHR(this->instance, this->window.surface, nullptr);
		vkDestroyInstance(this->instance, nullptr);
	}

	//helper(s)
	void ContextBase::CreateInstance()
	{
		assert(window.sdl_ptr != nullptr);

		//create instance info.
		VkInstanceCreateInfo createInfo = {};
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_3;
		appInfo.pApplicationName = "Caleb Vulkan Engine";
		appInfo.engineVersion = 1;
		appInfo.pNext = nullptr;

		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.flags = 0;

		//linked list of structures to pass to the create instance func.
		//--> look into it later.
		createInfo.pNext = nullptr;


		//we won't be doing any extension for now --> look into it at a later time.
		//need to get sdl extensionss
		unsigned int sdl_extensionCount = 0;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::init::DebugMessengerCreateInfo();

		if (SDL_Vulkan_GetInstanceExtensions(window.sdl_ptr, &sdl_extensionCount, nullptr) != SDL_TRUE)
		{
			throw std::runtime_error("could not grab extensions from SDL!");
		}

		std::vector<const char*> extensionNames(sdl_extensionCount);

		if (SDL_Vulkan_GetInstanceExtensions(window.sdl_ptr, &sdl_extensionCount, extensionNames.data()) != SDL_TRUE)
		{
			throw std::runtime_error("could not grab extensions from SDL!");
		}


		//find other instance extensions.
		uint32_t extensionPropertyCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, nullptr);

		std::vector<VkExtensionProperties> extensionProperties(extensionPropertyCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, extensionProperties.data());

		for (auto& property : extensionProperties)
		{
			if (strcmp(property.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
				extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
				break;
			}
		}


		//this could be useful for logging, profiling, debugging, whatever.
		//it intercepts the API
		if (vk::util::CheckValidationSupport())
		{
			createInfo.ppEnabledLayerNames = vk::instanceLayerExtensions;
			createInfo.enabledLayerCount = 1;
		}

		createInfo.enabledExtensionCount = extensionNames.size();
		createInfo.ppEnabledExtensionNames = extensionNames.data();

		createInfo.pNext = &debugCreateInfo;
		createInfo.pApplicationInfo = &appInfo;

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
			int(window.viewport.width), int(window.viewport.height), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

		if (window.sdl_ptr == nullptr)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
	}
	
	void ContextBase::ResizeWindow() 
	{
		assert(_Application != NULL);

		VK_CHECK_RESULT(vkDeviceWaitIdle(this->device.logical));

		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->device.physical, window.surface, &deviceCapabilities));

		currentExtent = deviceCapabilities.currentExtent;
		window.UpdateExtents(currentExtent);

		this->swapChain.Recreate(this->device.graphicsQueue.family,
			this->device.presentQueue.family, this->depthStencil, this->mPipeline.RenderPass(), window);
	}

	void ContextBase::FindQueueFamilies(const VkSurfaceKHR& windowSurface)
	{
		assert(device.physical);

		uint32_t queueFamilyPropertyCount;
		std::vector<VkQueueFamilyProperties> queueFamilies;

		//no use for memory properties right now.
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

		vkGetPhysicalDeviceMemoryProperties(device.physical, &physicalDeviceMemoryProperties);

		//similar maneuver to vkEnumeratePhysicalDevices
		vkGetPhysicalDeviceQueueFamilyProperties(device.physical, &queueFamilyPropertyCount, nullptr);

		if (queueFamilyPropertyCount == 0)
		{
			throw std::runtime_error("couldn't find any queue families...");
		}

		queueFamilies.resize(queueFamilyPropertyCount);

		vkGetPhysicalDeviceQueueFamilyProperties(device.physical, &queueFamilyPropertyCount, queueFamilies.data());

		bool setGraphicsQueue = false;
		bool setPresentQueue = false;

		for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
		{
			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				device.graphicsQueue.family = i;
				setGraphicsQueue = true;
			}


			VkBool32 presentSupport = false;
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(device.physical, i, windowSurface, &presentSupport));

			if (presentSupport)
			{
				device.presentQueue.family = i;
				setPresentQueue = true;
			}

			if (setGraphicsQueue && setPresentQueue)
			{
				break;
			}

		}
	}

	void ContextBase::EnumeratePhysicalDevices()
	{
		assert(this->instance != VK_NULL_HANDLE);

		std::vector<VkPhysicalDevice> gpus;
		int g_index = -1;

		//list the physical devices
		uint32_t max_devices = 0;

		//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices 
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(this->instance, &max_devices, nullptr))

		if (!max_devices)
		{
			throw std::runtime_error("could not find any GPUs to use!\n");
		}

		gpus.resize(max_devices);

		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(this->instance, &max_devices, gpus.data()))

		for (size_t i = 0; i < max_devices; ++i)
		{

			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;

			vkGetPhysicalDeviceProperties(gpus[i], &properties);
			vkGetPhysicalDeviceFeatures(gpus[i], &features);


			if ((properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
				properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) &&
				features.geometryShader && features.samplerAnisotropy)
			{
				std::cout << "picked device " << i << '\n';

				g_index = i;
				break;
			}
		}

		if (g_index < 0)
		{
			throw std::runtime_error("could not find suitable physical device!");
		}

		device.physical = gpus[g_index];
	}

	VkDevice ContextBase::CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily)
	{
		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos; //presentation and graphics.

		uint32_t uniqueQueueFamilies[2] = { graphicsFamily, presentFamily };

		float queuePriority[1] = { 1.f };

		if (graphicsFamily != presentFamily)
		{
			for (unsigned i = 0; i < 2; ++i)
			{
				VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.
				deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				deviceQueueInfo.flags = 0;
				deviceQueueInfo.pNext = nullptr;
				deviceQueueInfo.queueFamilyIndex = uniqueQueueFamilies[i];
				deviceQueueInfo.queueCount = 1;
				//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
				deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

				deviceQueueCreateInfos.push_back(deviceQueueInfo);
			}
		}
		else {
			VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.
			deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueInfo.flags = 0;
			deviceQueueInfo.pNext = nullptr;
			deviceQueueInfo.queueFamilyIndex = graphicsFamily;
			deviceQueueInfo.queueCount = 1;
			//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
			deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

			deviceQueueCreateInfos.push_back(deviceQueueInfo);

		}

		//won't do many other optional features for now.
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.geometryShader = VK_TRUE;
		/*	deviceFeatures.tessellationShader = VK_TRUE;*/
		deviceFeatures.samplerAnisotropy = VK_TRUE;


		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pNext = nullptr;

		//maybe don't assume extensions are there!!!!	
		deviceCreateInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
		deviceCreateInfo.ppEnabledExtensionNames = vk::deviceExtensions;

		deviceCreateInfo.pEnabledFeatures = &deviceFeatures; //call vkGetPhysicalDeviceFeatures to set additional features.

		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = (uint32_t)(deviceQueueCreateInfos.size());

		VkDevice nLogicalDevice;
		VK_CHECK_RESULT(vkCreateDevice(p_device, &deviceCreateInfo, nullptr, &nLogicalDevice));

		return nLogicalDevice;
	}

	//initializers
	
	void ContextBase::InitializeDepthStencil() 
	{
		this->depthStencil = vk::rsc::CreateDepthResources(device.physical, device.logical, window.viewport);
	}

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
		attachments[1].format = depthStencil.format;
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


		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = dependencies[0].srcStageMask;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;

		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = dependencies[1].srcStageMask;
		dependencies[1].srcAccessMask = 0;
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

		VK_CHECK_RESULT(vkCreateRenderPass(device.logical, &renderPassCI, nullptr, &this->mPipeline.mRenderPass));
	}

	//constructor
	ContextBase::ContextBase()
	{
		assert(_Application != NULL);

		ContextBase::CreateWindow();
		ContextBase::CreateInstance();

		if (SDL_Vulkan_CreateSurface(window.sdl_ptr, instance, &window.surface) != SDL_TRUE)
		{
			throw std::runtime_error("could not create window surface! " + std::string(SDL_GetError()));
		}
		
		ContextBase::EnumeratePhysicalDevices();
		vkGetPhysicalDeviceMemoryProperties(device.physical, &device.memoryProperties);

		ContextBase::FindQueueFamilies(window.surface);

		this->device.logical = ContextBase::CreateLogicalDevice(this->device.physical, device.graphicsQueue.family, device.presentQueue.family);

		vkGetDeviceQueue(device.logical, device.graphicsQueue.family, 0, &device.graphicsQueue.handle);
		vkGetDeviceQueue(device.logical, device.presentQueue.family, 0, &device.presentQueue.handle);

		semaphores.presentComplete = vk::init::CreateSemaphore(this->device.logical);
		semaphores.renderComplete = vk::init::CreateSemaphore(this->device.logical);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &pipelineWaitStages;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphores.renderComplete;

		this->swapChain = SwapChain(&this->device, device.graphicsQueue.family, device.presentQueue.family, window.surface);
		
		ContextBase::InitializeDepthStencil();
		ContextBase::InitializeRenderPass();

		this->commandPool = vk::init::CommandPool(device.logical, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physical, window.surface, &deviceCapabilities));

		//size of command buffer array is the same as swap chain image array
		for (int i = 0; i < deviceCapabilities.minImageCount + 1; ++i)
		{
			this->commandBuffers.push_back(vk::init::CommandBuffer(device.logical, this->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		}

		this->currentExtent = deviceCapabilities.currentExtent;

		this->mHotReloader.appDevicePtr = this->device.logical;
		this->mHotReloader.AddPipeline(this->mPipeline);

		this->isInitialized = true;

	}

	//getter(s)
	vk::Queue ContextBase::GraphicsQueue()
	{
		return this->device.graphicsQueue;
	}

	const VkPipeline ContextBase::Pipeline() const
	{
		return this->mPipeline.Handle();
	}

	const VkPhysicalDevice ContextBase::PhysicalDevice() const 
	{
		return this->device.physical;
	}

	const VkDevice ContextBase::LogicalDevice() const 
	{
		return this->device.logical;
	}

	vk::UniformTransform& ContextBase::SceneTransform() 
	{
		return this->uTransform;
	}

	VkDescriptorPool ContextBase::DescriptorPool() const 
	{
		return this->descriptorPool;
	}

	void ContextBase::WaitForDevice()
	{
		if (this->isInitialized) 
		{
			VK_CHECK_RESULT(vkDeviceWaitIdle(this->device.logical));
		}
	}

	void ContextBase::Render()
	{
		//wait for queue submission..
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device.logical, swapChain.handle, UINT64_MAX, semaphores.presentComplete, (VkFence)nullptr, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			ResizeWindow();
			return;
		}
		else 
		{
			assert(result == VK_SUCCESS);
		}

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &this->commandBuffers[imageIndex];

		VK_CHECK_RESULT(vkQueueSubmit(this->device.graphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE))

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &this->swapChain.handle;
		presentInfo.pImageIndices = &imageIndex;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &semaphores.renderComplete;

		result = vkQueuePresentKHR(this->device.presentQueue.handle, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			ResizeWindow();
			return;
		}
		else 
		{
			assert(result == VK_SUCCESS);
		}

		//so we wait a bit here on the CPU for every submission. This is inefficient, but good enough for now.
		VK_CHECK_RESULT(vkQueueWaitIdle(this->device.presentQueue.handle));
	}

}