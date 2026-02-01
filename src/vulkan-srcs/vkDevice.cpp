

#include "vkDevice.h"
#include "vkInit.h"
#include <stdexcept>
#include <cassert>

namespace vk 
{
	
	void Device::Init( VkInstance instance, VkSurfaceKHR windowSurface )
	{
		assert(instance != VK_NULL_HANDLE && windowSurface != VK_NULL_HANDLE);

		FindPhysicalDevices(instance);
		
		CheckRequestedExtensions();

		//device properties
		vkGetPhysicalDeviceMemoryProperties(this->physical, &memoryProperties);
		
		//descriptor buffer info
		VkPhysicalDeviceProperties2KHR deviceProperties2 = {};
		descriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		deviceProperties2.pNext = &descriptorBufferProperties;
		vkGetPhysicalDeviceProperties2(this->physical, &deviceProperties2);

		assert(descriptorBufferProperties.maxDescriptorBufferBindings >= 2);

		FindQueueFamilies(windowSurface);

		InitializeLogicalDevice();

		vkGetDeviceQueue(this->logical, graphicsQueue.family, 0, &graphicsQueue.handle);
		vkGetDeviceQueue(this->logical, presentQueue.family, 0, &presentQueue.handle);
		vkGetDeviceQueue(this->logical, transferQueue.family, 0, &transferQueue.handle);

		g_vkGetDescriptorSetLayoutBindingOffsetEXT =
			(PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)
			(vkGetDeviceProcAddr(logical, "vkGetDescriptorSetLayoutBindingOffsetEXT"));
		
		g_vkGetDescriptorSetLayoutSizeEXT = 
			(PFN_vkGetDescriptorSetLayoutSizeEXT)
			(vkGetDeviceProcAddr(logical, "vkGetDescriptorSetLayoutSizeEXT"));
		
		g_vkGetDescriptorEXT = 
			(PFN_vkGetDescriptorEXT)(vkGetDeviceProcAddr(logical, "vkGetDescriptorEXT"));
		
		g_vkCmdBindDescriptorBuffersEXT = 
			(PFN_vkCmdBindDescriptorBuffersEXT)(vkGetDeviceProcAddr(logical, "vkCmdBindDescriptorBuffersEXT"));
		
		g_vkCmdSetDescriptorBufferOffsetsEXT = 
			(PFN_vkCmdSetDescriptorBufferOffsetsEXT)(vkGetDeviceProcAddr(logical, "vkCmdSetDescriptorBufferOffsetsEXT"));

		assert(g_vkGetDescriptorSetLayoutSizeEXT);
		assert(g_vkGetDescriptorSetLayoutBindingOffsetEXT);
		assert(g_vkGetDescriptorEXT);
		assert(g_vkCmdBindDescriptorBuffersEXT);
		assert(g_vkCmdSetDescriptorBufferOffsetsEXT);

		this->commandPool = vk::init::CommandPool(this->logical,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsQueue.family);
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


			if ((properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
				features.geometryShader && features.samplerAnisotropy)
			{
				std::cout << "picked device " << i << '\n';

				g_index = static_cast<int>(i);
				break;
			}
		}

		if (g_index < 0)
		{
			throw std::runtime_error("could not find suitable physical device!");
		}

		this->physical = gpus[g_index];

	}

	void Device::FindQueueFamilies( VkSurfaceKHR windowSurface )
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
		bool setTransferQueue = false;

		for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
		{
			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
				(queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) 
			{
				transferQueue.family = i;
				setTransferQueue = true;
			}

			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				graphicsQueue.family = i;
				setGraphicsQueue = true;

				VkBool32 presentSupport = false;
				VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(this->physical, i, windowSurface, &presentSupport));
				if (presentSupport)
				{
					presentQueue.family = i;
					setPresentQueue = true;
				}
			}

			if (setGraphicsQueue && 
				setPresentQueue && 
				setTransferQueue)
			{
				break;
			}

		}

		if (!setGraphicsQueue || 
			!setPresentQueue || 
			!setTransferQueue)
		{
			throw std::runtime_error("could not find all required queues on this device!\n");
		}

		if (presentQueue.family != graphicsQueue.family)
		{
			std::cerr << "present queue and graphics queue families are different. Not supported in this code base.\n";
			throw std::runtime_error("vk::Device() failed!\n");
		}

	}

	void Device::InitializeLogicalDevice()
	{
		assert(graphicsQueue.family != -1 && presentQueue.family != -1);

		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos; //presentation and graphics.

		std::array<uint32_t, 3> uniqueQueueFamilies = { graphicsQueue.family, presentQueue.family, transferQueue.family };

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

		//transfer queue addition.
		VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.
		deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueInfo.flags = 0;
		deviceQueueInfo.pNext = nullptr;
		deviceQueueInfo.queueFamilyIndex = transferQueue.family;
		deviceQueueInfo.queueCount = 1;
		//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
		deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

		deviceQueueCreateInfos.push_back(deviceQueueInfo);

		

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pNext = nullptr;

		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = requestedExtensions.data();

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.geometryShader = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures; //call vkGetPhysicalDeviceFeatures to set additional features.

		//enabling descriptor buffers
		VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_feature = {};
		descriptor_buffer_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT;
		descriptor_buffer_feature.descriptorBuffer = VK_TRUE;

		VkPhysicalDeviceBufferDeviceAddressFeaturesEXT buffer_device_address_feature = {};
		buffer_device_address_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
		buffer_device_address_feature.bufferDeviceAddress = VK_TRUE;
		buffer_device_address_feature.pNext = &descriptor_buffer_feature;

		deviceCreateInfo.pNext = &buffer_device_address_feature;

		
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = (uint32_t)(deviceQueueCreateInfos.size());

		VK_CHECK_RESULT(vkCreateDevice(this->physical, &deviceCreateInfo, nullptr, &this->logical));

	}

	void Device::CheckRequestedExtensions() 
	{
		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(this->physical, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> supportedExtensions(extension_count);
		vkEnumerateDeviceExtensionProperties(this->physical, nullptr, &extension_count, supportedExtensions.data());

		for (auto& e : requestedExtensions) 
		{
			bool foundExtension = false;
			for (auto& se : supportedExtensions) 
			{
				if (strcmp(se.extensionName, e) == 0) 
				{
					foundExtension = true;
					break;
				}
			}

			assert(foundExtension == true); //TODO: write a logger to report generic errors like this.
		}
	}

	uint32_t Device::GetMemoryType( uint32_t typeBits, VkMemoryPropertyFlags properties )
	{

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

	VkPhysicalDeviceDescriptorBufferPropertiesEXT Device::DescriptorBufferProperties() const
	{
		return descriptorBufferProperties;
	}

	Buffer Device::CreateBuffer( size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data )
	{
		return Buffer(this, usage, flags, size, data);
	}

	VkCommandBuffer Device::CreateCommandBuffer( VkCommandBufferLevel level, bool begin )
	{

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vk::init::CommandBufferAllocateInfo();
		cmdBufAllocateInfo.commandPool = commandPool;
		cmdBufAllocateInfo.level = level;
		cmdBufAllocateInfo.commandBufferCount = 1;
		
		VkCommandBuffer cmdBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(this->logical, &cmdBufAllocateInfo, &cmdBuffer));

		if (begin) 
		{
			VkCommandBufferBeginInfo cmdBufInfo = vk::init::CommandBufferBeginInfo();
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
		}

		return cmdBuffer;
	}

	void Device::FlushCommandBuffer( VkCommandBuffer cmdBuffer, VkQueue queue, VkCommandPool pool, bool free )
	{
		//create a fence, submit the work to the gpu, and then delete the fence and free the command buffer
		VkSubmitInfo submitInfo = vk::init::SubmitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer)); 

		VkFenceCreateInfo fenceCI = vk::init::FenceCreateInfo();
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(this->logical, &fenceCI, nullptr, &fence));	

		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));

		VK_CHECK_RESULT(vkWaitForFences(this->logical, 1, &fence, VK_TRUE, UINT64_MAX));

		vkDestroyFence(this->logical, fence, nullptr);

		if (free) 
		{
			vkFreeCommandBuffers(this->logical, pool, 1, &cmdBuffer);
			cmdBuffer = VK_NULL_HANDLE;
		}

	}

	void Device::AddExtension( const char* name )
	{
		//gotta save memory.
		for (auto& extension : requestedExtensions) 
		{
			if (strcmp(extension, name) == 0)
			{
				return;
			}
		}

		requestedExtensions.push_back(name);
	}
}
