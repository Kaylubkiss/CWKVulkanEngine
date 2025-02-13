#include "vkGraphicsSystem.h"
#include "vkUtility.h"
#include <stdexcept>

namespace vk
{
	void RenderResources::Destroy(const VkDevice l_device) 
	{
		vkDestroyPipeline(l_device, this->defaultPipeline, nullptr);

		vkDestroyRenderPass(l_device,this->renderPass, nullptr);
		
		vkDestroyFence(l_device, this->inFlightFence, nullptr);
		
		//command pools and their handles.
		vkFreeCommandBuffers(l_device, this->commandPool, 1, &this->commandBuffer);
		vkDestroyCommandPool(l_device, this->commandPool, nullptr);
		
		//semaphores
		vkDestroySemaphore(l_device, this->imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(l_device, this->renderFinishedSemaphore, nullptr);
	}

	GraphicsSystem::~GraphicsSystem()
	{
		//don't need to delete physical device
		renderResources.Destroy(this->logicalGpu);
		swapChain.Destroy(this->logicalGpu);


		vkDestroyDevice(this->logicalGpu, nullptr);

		delete [] gpus;
	}

	GraphicsSystem::GraphicsSystem(const VkInstance vkInstance, const VkSurfaceKHR windowSurface)
	{
		if (vkInstance == VK_NULL_HANDLE) {

			throw std::runtime_error("Can't create graphics without instance!");
		}

		GraphicsSystem::EnumeratePhysicalDevices(vkInstance);
		GraphicsSystem::FindQueueFamilies(this->gpus[g_index], windowSurface);

		this->logicalGpu = GraphicsSystem::CreateLogicalDevice(this->gpus[g_index], graphicsQueue.family, presentQueue.family);

		vkGetDeviceQueue(this->logicalGpu, graphicsQueue.family, 0, &graphicsQueue.handle);
		vkGetDeviceQueue(this->logicalGpu, presentQueue.family, 0, &presentQueue.handle);

	}

	const VkPhysicalDevice GraphicsSystem::PhysicalDevice() const
	{
		return this->gpus[g_index];
	}

	void GraphicsSystem::FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface)
	{

		uint32_t queueFamilyPropertyCount;
		VkQueueFamilyProperties* queueFamilies = nullptr;

		//no use for memory properties right now.
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

		//similar maneuver to vkEnumeratePhysicalDevices
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

		if (queueFamilyPropertyCount == 0)
		{
			throw std::runtime_error("couldn't find any queue families...");
		}

		queueFamilies = new VkQueueFamilyProperties[queueFamilyPropertyCount];

		if (queueFamilies == nullptr)
		{
			throw std::runtime_error("couldn't allocate queueFamilies array\n");
		}

		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilies);

		bool setGraphicsQueue = false;
		bool setPresentQueue = false;

		for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
		{
			if (((*(queueFamilies + i)).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
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

		delete[] queueFamilies;
	}

	void GraphicsSystem::EnumeratePhysicalDevices(const VkInstance& vkInstance)
	{
		//list the physical devices
		uint32_t max_devices = 0;

		//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices 
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &max_devices, nullptr))

		if (max_devices <= 0)
		{
			throw std::runtime_error("could not find any GPUs to use!\n");
			return;
		}


		this->gpus = new VkPhysicalDevice[max_devices];

		if (this->gpus == NULL)
		{
			throw std::runtime_error("could not allocate array of physical devices\n");
		}

		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &max_devices, this->gpus))

		for (size_t i = 0; i < max_devices; ++i)
		{
			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;

			vkGetPhysicalDeviceProperties(*(this->gpus + i), &properties);
			vkGetPhysicalDeviceFeatures(*(this->gpus + i), &features);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
			features.geometryShader && features.samplerAnisotropy)
			{
				g_index = i;
			}
		}

		if (g_index < 0)
		{
			throw std::runtime_error("could not find suitable physical device!");
		}


	}

	VkDevice GraphicsSystem::CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfos[2]; //presentation and graphics.

		uint32_t uniqueQueueFamilies[2] = { graphicsFamily, presentFamily };

		for (unsigned i = 0; i < 2; ++i)
		{
			VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.
			deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueInfo.flags = 0;
			deviceQueueInfo.pNext = nullptr;
			deviceQueueInfo.queueFamilyIndex = uniqueQueueFamilies[i];
			deviceQueueInfo.queueCount = 1;
			float queuePriority = 1.f;
			//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
			deviceQueueInfo.pQueuePriorities = &queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

			deviceQueueCreateInfos[i] = deviceQueueInfo;
		}

		//won't do many other optional features for now.
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.geometryShader = VK_TRUE;
		deviceFeatures.tessellationShader = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pNext = nullptr;

		//layers are no right now.

		deviceCreateInfo.enabledLayerCount = 1;
		deviceCreateInfo.ppEnabledLayerNames = enabledLayerNames;

		deviceCreateInfo.enabledExtensionCount = 1;
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

		deviceCreateInfo.pEnabledFeatures = &deviceFeatures; //call vkGetPhysicalDeviceFeatures to set additional features.

		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;
		deviceCreateInfo.queueCreateInfoCount = 1;

		VkDevice nLogicalDevice;
		VK_CHECK_RESULT(vkCreateDevice(p_device, &deviceCreateInfo, nullptr, &nLogicalDevice));

		return nLogicalDevice;
	}

	const VkDevice GraphicsSystem::LogicalDevice() const 
	{
		return this->logicalGpu;
	}

}