#pragma once

namespace vk 
{
	struct Queue
	{
		VkQueue handle = VK_NULL_HANDLE;
		uint32_t family = uint32_t(-1);
	};

	//TODO: make these members private and turn device into a class.
	struct Device 
	{
		//data
		VkPhysicalDevice physical = VK_NULL_HANDLE;
		VkDevice logical          = VK_NULL_HANDLE;

		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
		vk::Queue transferQueue;

		VkCommandPool commandPool = VK_NULL_HANDLE;

	public:
		Device() = default;
		~Device() = default;
		void Init(VkInstance instance, VkSurfaceKHR surface);
		void Destroy();
		//functionality
		uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);
		VkPhysicalDeviceDescriptorBufferPropertiesEXT DescriptorBufferProperties() const;
		Buffer CreateBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data);
		VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin);
		void FlushCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue, VkCommandPool pool, bool free);
		void AddExtension(const char* name);
	//helpers
	private:
		void FindPhysicalDevices(VkInstance instance);
		void FindQueueFamilies(VkSurfaceKHR windowSurface);
		void InitializeLogicalDevice();
		void CheckRequestedExtensions();
	private:
		std::vector<const char*> requestedExtensions;
		//properties
		VkPhysicalDeviceMemoryProperties memoryProperties = {};
		VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProperties = {};

	};

}

