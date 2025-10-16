

#include "vkDevice.h"
#include "vkInit.h"
#include <stdexcept>
#include <cassert>

namespace vk 
{
	
	void Device::Initialize(VkInstance instance, VkSurfaceKHR windowSurface) 
	{
		assert(instance != VK_NULL_HANDLE && windowSurface != VK_NULL_HANDLE);

		FindPhysicalDevices(instance);
		vkGetPhysicalDeviceMemoryProperties(this->physical, &memoryProperties);
		FindQueueFamilies(windowSurface);
		InitializeLogicalDevice();

		vkGetDeviceQueue(this->logical, graphicsQueue.family, 0, &graphicsQueue.handle);
		vkGetDeviceQueue(this->logical, presentQueue.family, 0, &presentQueue.handle);
	}

	void Device::Destroy() 
	{
		vkDestroyDevice(this->logical, nullptr);
	}

	void Device::FindPhysicalDevices(VkInstance instance) 
	{
		assert(instance != VK_NULL_HANDLE);

		std::vector<VkPhysicalDevice> gpus;
		int g_index = -1;

		//list the physical devices
		uint32_t max_devices = 0;

		//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices 
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &max_devices, nullptr))

		if (!max_devices)
		{
			throw std::runtime_error("could not find any GPUs to use!\n");
		}

		gpus.resize(max_devices);

		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &max_devices, gpus.data()))

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

		this->physical = gpus[g_index];

	}

	void Device::FindQueueFamilies(VkSurfaceKHR windowSurface) 
	{
		uint32_t queueFamilyPropertyCount;
		std::vector<VkQueueFamilyProperties> queueFamilies;

		//no use for memory properties right now.
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

		vkGetPhysicalDeviceMemoryProperties(this->physical, &physicalDeviceMemoryProperties);

		//similar maneuver to vkEnumeratePhysicalDevices
		vkGetPhysicalDeviceQueueFamilyProperties(this->physical, &queueFamilyPropertyCount, nullptr);

		if (queueFamilyPropertyCount == 0)
		{
			throw std::runtime_error("couldn't find any queue families...");
		}

		queueFamilies.resize(queueFamilyPropertyCount);

		vkGetPhysicalDeviceQueueFamilyProperties(this->physical, &queueFamilyPropertyCount, queueFamilies.data());

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
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(this->physical, i, windowSurface, &presentSupport));

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

		if (!setGraphicsQueue || !setPresentQueue)
		{
			throw std::runtime_error("could not find all required queues on this device!\n");
		}

	}

	void Device::InitializeLogicalDevice()
	{
		assert(graphicsQueue.family != -1 && presentQueue.family != -1);

		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos; //presentation and graphics.

		uint32_t uniqueQueueFamilies[2] = { graphicsQueue.family, presentQueue.family };

		float queuePriority[1] = { 1.f };

		if (uniqueQueueFamilies[0] != uniqueQueueFamilies[1])
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
			deviceQueueInfo.queueFamilyIndex = graphicsQueue.family;
			deviceQueueInfo.queueCount = 1;
			//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
			deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

			deviceQueueCreateInfos.push_back(deviceQueueInfo);

		}

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pNext = nullptr;


		static const char* deviceExtensions[1] =
		{
			"VK_KHR_swapchain"
		};

		//maybe don't assume extensions are there!!!!	
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(deviceExtensions));
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.geometryShader = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		deviceCreateInfo.pEnabledFeatures = &deviceFeatures; //call vkGetPhysicalDeviceFeatures to set additional features.

		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = (uint32_t)(deviceQueueCreateInfos.size());

		VK_CHECK_RESULT(vkCreateDevice(this->physical, &deviceCreateInfo, nullptr, &this->logical));

	}



	uint32_t Device::GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) {

		for (uint32_t i = 0; i < this->memoryProperties.memoryTypeCount; ++i)
		{
			if ((typeBits & 1) == 1)
			{
				if ((this->memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}

			typeBits >>= 1;
		}


		throw std::runtime_error("couldn't find the requested memory type on device");
	}

	Buffer Device::CreateBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data) 
	{
		return Buffer(this->physical,this->logical, size, usage, flags, data);
	}

	FramebufferAttachment Device::CreateFramebufferAttachment(const VkViewport& viewport, VkImageUsageFlagBits usage, VkFormat format) 
	{
		VkImageAspectFlags aspectMask = 0;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) 
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		}
		else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) 
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (format == VK_FORMAT_UNDEFINED)
			{
				format = vk::util::findSupportedFormat(Device::physical,
					{ VK_FORMAT_D32_SFLOAT_S8_UINT,
					VK_FORMAT_D32_SFLOAT,
					VK_FORMAT_D24_UNORM_S8_UINT,
					VK_FORMAT_D16_UNORM_S8_UINT,
					VK_FORMAT_D16_UNORM },
					VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
				);
			}

			if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
			{
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}

		assert(format != VK_FORMAT_UNDEFINED);


		FramebufferAttachment nDepthResources = {};
		nDepthResources.format = format;

		uint32_t mipLevels = 1;

		nDepthResources.image = vk::init::CreateImage(Device::physical, Device::logical, 
			(uint32_t)viewport.width, (uint32_t)viewport.height, mipLevels,
			format, 
			usage | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			nDepthResources.imageMemory);

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = nDepthResources.image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange = { aspectMask, 0, 1, 0, 1 };

		VK_CHECK_RESULT(vkCreateImageView(Device::logical, &viewInfo, nullptr, &nDepthResources.imageView));

		return nDepthResources;

	}

}