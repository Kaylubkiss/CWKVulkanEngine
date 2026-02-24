

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
		vkGetPhysicalDeviceMemoryProperties(m_gpu, &m_memoryProperties);
		
		//descriptor buffer info
		VkPhysicalDeviceProperties2KHR deviceProperties2 = {};
		m_descriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		deviceProperties2.pNext = &m_descriptorBufferProperties;
		vkGetPhysicalDeviceProperties2(m_gpu, &deviceProperties2);

		assert(m_descriptorBufferProperties.maxDescriptorBufferBindings >= 2);

		FindQueueFamilies(windowSurface);

		InitializeLogicalDevice();


		for (auto& queue : m_queues)
		{
			vkGetDeviceQueue(m_device, queue.family, 0, &queue.handle);
		}

		g_vkGetDescriptorSetLayoutBindingOffsetEXT =
			(PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)
			(vkGetDeviceProcAddr(m_device, "vkGetDescriptorSetLayoutBindingOffsetEXT"));
		
		g_vkGetDescriptorSetLayoutSizeEXT = 
			(PFN_vkGetDescriptorSetLayoutSizeEXT)
			(vkGetDeviceProcAddr(m_device, "vkGetDescriptorSetLayoutSizeEXT"));
		
		g_vkGetDescriptorEXT = 
			(PFN_vkGetDescriptorEXT)(vkGetDeviceProcAddr(m_device, "vkGetDescriptorEXT"));
		
		g_vkCmdBindDescriptorBuffersEXT = 
			(PFN_vkCmdBindDescriptorBuffersEXT)(vkGetDeviceProcAddr(m_device, "vkCmdBindDescriptorBuffersEXT"));
		
		g_vkCmdSetDescriptorBufferOffsetsEXT = 
			(PFN_vkCmdSetDescriptorBufferOffsetsEXT)(vkGetDeviceProcAddr(m_device, "vkCmdSetDescriptorBufferOffsetsEXT"));

		assert(g_vkGetDescriptorSetLayoutSizeEXT);
		assert(g_vkGetDescriptorSetLayoutBindingOffsetEXT);
		assert(g_vkGetDescriptorEXT);
		assert(g_vkCmdBindDescriptorBuffersEXT);
		assert(g_vkCmdSetDescriptorBufferOffsetsEXT);

		m_commandPool = vk::init::CommandPool(m_device,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_queues[DeviceQueue::GRAPHICS].family);
	}

	void Device::Destroy()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			if (m_commandPool != VK_NULL_HANDLE)
			{
				vkDestroyCommandPool(m_device, m_commandPool, nullptr);
				m_commandPool = VK_NULL_HANDLE;
			}

			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}
	}

	void Device::FindPhysicalDevices(VkInstance instance) 
	{
		assert(instance != VK_NULL_HANDLE);

		std::vector<VkPhysicalDevice> gpus;
		int g_index = -1;

		//list the physical devices
		uint32_t max_devices = 0;

		//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices 
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &max_devices, nullptr));

		if (!max_devices)
		{
			throw std::runtime_error("could not find any GPUs to use!\n");
		}

		gpus.resize(max_devices);

		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &max_devices, gpus.data()));

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

		m_gpu = gpus[g_index];

	}

	void Device::FindQueueFamilies( VkSurfaceKHR windowSurface )
	{
		uint32_t queueFamilyPropertyCount;
		std::vector<VkQueueFamilyProperties> queueFamilies;

		//no use for memory properties right now.
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

		vkGetPhysicalDeviceMemoryProperties(m_gpu, &physicalDeviceMemoryProperties);

		//similar maneuver to vkEnumeratePhysicalDevices
		vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &queueFamilyPropertyCount, nullptr);

		if (queueFamilyPropertyCount == 0)
		{
			throw std::runtime_error("couldn't find any queue families...");
		}

		queueFamilies.resize(queueFamilyPropertyCount);

		vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &queueFamilyPropertyCount, queueFamilies.data());

		bool setGraphicsQueue = false;
		bool setPresentQueue = false;
		bool setTransferQueue = false;

		for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
		{
			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
				(queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) 
			{
				m_queues[DeviceQueue::TRANSFER].family = i;
				setTransferQueue = true;
			}

			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				m_queues[DeviceQueue::GRAPHICS].family = i;
				setGraphicsQueue = true;

				VkBool32 presentSupport = false;
				VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, i, windowSurface, &presentSupport));
				if (presentSupport)
				{
					m_queues[DeviceQueue::PRESENT].family = i;
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

		if (m_queues[DeviceQueue::PRESENT].family != m_queues[DeviceQueue::GRAPHICS].family)
		{
			std::cerr << "present queue and graphics queue families are different. Not supported in this code base.\n";
			throw std::runtime_error("vk::Device() failed!\n");
		}

	}

	void Device::InitializeLogicalDevice()
	{
		assert(m_queues[DeviceQueue::GRAPHICS].family != -1 && m_queues[DeviceQueue::PRESENT].family != -1);

		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos; //presentation and graphics.
		std::vector<uint32_t> uniqueQueueFamilies;
		uniqueQueueFamilies.reserve(m_queues.size());

		for (auto& queue : m_queues)
		{
			uniqueQueueFamilies.push_back(queue.family);
		}

		float queuePriority[1] = { 1.f }; //each queue gets an equal amount of time to work.

		VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.

		if (uniqueQueueFamilies[DeviceQueue::GRAPHICS] != uniqueQueueFamilies[DeviceQueue::PRESENT])
		{
			for (unsigned i = 0; i < 2; ++i)
			{
				deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				deviceQueueInfo.flags = 0;
				deviceQueueInfo.pNext = nullptr;
				deviceQueueInfo.queueFamilyIndex = uniqueQueueFamilies[i];
				deviceQueueInfo.queueCount = 1;
				deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

				deviceQueueCreateInfos.push_back(deviceQueueInfo);
			}
		}
		else
		{
			deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueInfo.queueFamilyIndex = m_queues[DeviceQueue::GRAPHICS].family;
			deviceQueueInfo.queueCount = 1;
			//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
			deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

			deviceQueueCreateInfos.push_back(deviceQueueInfo);
		}

		//transfer queue addition.
		deviceQueueInfo = {};
		deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueInfo.flags = 0;
		deviceQueueInfo.pNext = nullptr;
		deviceQueueInfo.queueFamilyIndex = m_queues[DeviceQueue::TRANSFER].family;
		deviceQueueInfo.queueCount = 1;
		deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

		deviceQueueCreateInfos.push_back(deviceQueueInfo);

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pNext = nullptr;

		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_requestedExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = m_requestedExtensions.data();

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

		VK_CHECK_RESULT(vkCreateDevice(m_gpu, &deviceCreateInfo, nullptr, &m_device));
	}

	void Device::CheckRequestedExtensions() const
	{
		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> supportedExtensions(extension_count);
		vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &extension_count, supportedExtensions.data());

		for (auto& e : m_requestedExtensions)
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

	uint32_t Device::GetMemoryType( uint32_t typeBits, VkMemoryPropertyFlags properties ) const
	{

		for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; ++i)
		{
			if ((typeBits & 1) == 1)
			{
				if ((m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}

			typeBits >>= 1;
		}


		throw std::runtime_error("couldn't find the requested memory type on device");
	}

	VkPhysicalDeviceDescriptorBufferPropertiesEXT Device::GetDescriptorBufferProperties() const
	{
		return m_descriptorBufferProperties;
	}

	const VkDevice Device::GetDevice() const
	{
		return m_device;
	}

	const VkPhysicalDevice Device::GetGPU() const
	{
		return m_gpu;
	}

	const vk::Queue& Device::GetQueue( DeviceQueue queue ) const
	{
		//NOTE: queue is an enum namespace, so these operators work.
		if (queue < m_queues.size())
		{
			return m_queues[queue];
		}
		else
		{
			std::cerr << "\033[31m" << "requested queue is outside the range of supported queues" << "\033[0m" << "\n";
			throw std::runtime_error("Device::GetQueue() Failed!");
		}

		return m_queues[0];
	}

	Buffer Device::CreateBuffer( size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data )
	{
		return Buffer(this, usage, flags, size, data);
	}

	void Device::AllocateCommandBuffers( VkCommandBuffer* commandBuffers, uint32_t count ) const
	{
		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = vk::init::CommandBufferAllocateInfo();
		cmdBufferAllocateInfo.commandBufferCount = count;
		cmdBufferAllocateInfo.commandPool = m_commandPool;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &cmdBufferAllocateInfo, commandBuffers));
	}

	void Device::FreeCommandBuffers( const VkCommandBuffer* commandBuffers, uint32_t count ) const
	{
		vkFreeCommandBuffers(m_device, m_commandPool, count, commandBuffers);
	}

	VkCommandBuffer Device::CreateCommandBuffer( VkCommandBufferLevel level, bool begin )
	{

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vk::init::CommandBufferAllocateInfo();
		cmdBufAllocateInfo.commandPool = m_commandPool;
		cmdBufAllocateInfo.level = level;
		cmdBufAllocateInfo.commandBufferCount = 1;
		
		VkCommandBuffer cmdBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &cmdBuffer));

		if (begin) 
		{
			VkCommandBufferBeginInfo cmdBufInfo = vk::init::CommandBufferBeginInfo();
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
		}

		return cmdBuffer;
	}

	void Device::FlushCommandBuffer( VkCommandBuffer cmdBuffer, VkQueue queue, bool free )
	{
		//create a fence, submit the work to the gpu, and then delete the fence and free the command buffer
		VkSubmitInfo submitInfo = vk::init::SubmitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer)); 

		VkFenceCreateInfo fenceCI = vk::init::FenceCreateInfo();
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(m_device, &fenceCI, nullptr, &fence));

		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));

		VK_CHECK_RESULT(vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX));

		vkDestroyFence(m_device, fence, nullptr);

		if (free) 
		{
			vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmdBuffer);
			cmdBuffer = VK_NULL_HANDLE;
		}

	}

	void Device::AddExtension( const char* name )
	{
		//gotta save memory.
		for (auto& extension : m_requestedExtensions)
		{
			if (strcmp(extension, name) == 0)
			{
				return;
			}
		}

		m_requestedExtensions.push_back(name);
	}
}
