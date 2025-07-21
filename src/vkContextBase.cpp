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
		swapChain.Destroy(this->device.logical);

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
	
	void ContextBase::FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface)
	{

		uint32_t queueFamilyPropertyCount;
		std::vector<VkQueueFamilyProperties> queueFamilies;

		//no use for memory properties right now.
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

		//similar maneuver to vkEnumeratePhysicalDevices
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

		if (queueFamilyPropertyCount == 0)
		{
			throw std::runtime_error("couldn't find any queue families...");
		}

		queueFamilies.resize(queueFamilyPropertyCount);

		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilies.data());

		bool setGraphicsQueue = false;
		bool setPresentQueue = false;

		for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
		{
			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				graphicsQueue.family = i;
				setGraphicsQueue = true;
			}


			VkBool32 presentSupport = false;
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport));

			if (presentSupport)
			{
				presentQueue.family = i;
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
		ContextBase::FindQueueFamilies(this->device.physical, window.surface);

		this->device.logical = ContextBase::CreateLogicalDevice(this->device.physical, graphicsQueue.family, presentQueue.family);

		vkGetDeviceQueue(this->device.logical, graphicsQueue.family, 0, &graphicsQueue.handle);
		vkGetDeviceQueue(this->device.logical, presentQueue.family, 0, &presentQueue.handle);

		semaphores.presentComplete = vk::init::CreateSemaphore(this->device.logical);
		semaphores.renderComplete = vk::init::CreateSemaphore(this->device.logical);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &pipelineWaitStages;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphores.renderComplete;

		this->swapChain = SwapChain(this->device.logical, this->device.physical, graphicsQueue.family, presentQueue.family, window.surface);

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
		return this->graphicsQueue;
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

	const VkDescriptorSetLayout ContextBase::DescriptorSetLayout() const
	{
		//TODO: support different descriptorsetlayouts.
		return this->mPipeline.DescriptorSetLayout();
	}

	
	//update(s)
	void ContextBase::BindPipelineLayoutToObject(Object& obj)
	{
		obj.UpdatePipelineLayout(this->mPipeline.Layout());
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

		VK_CHECK_RESULT(vkQueueSubmit(this->graphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE))

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &this->swapChain.handle;
		presentInfo.pImageIndices = &imageIndex;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &semaphores.renderComplete;

		result = vkQueuePresentKHR(this->presentQueue.handle, &presentInfo);

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
		VK_CHECK_RESULT(vkQueueWaitIdle(this->presentQueue.handle));
	}

}