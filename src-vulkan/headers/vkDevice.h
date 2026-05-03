#pragma once
#include "vkBuffer.h"

extern PFN_vkGetDescriptorSetLayoutBindingOffsetEXT g_vkGetDescriptorSetLayoutBindingOffsetEXT;
extern PFN_vkGetDescriptorSetLayoutSizeEXT g_vkGetDescriptorSetLayoutSizeEXT;
extern PFN_vkGetDescriptorEXT g_vkGetDescriptorEXT;
extern PFN_vkCmdBindDescriptorBuffersEXT g_vkCmdBindDescriptorBuffersEXT;
extern PFN_vkCmdSetDescriptorBufferOffsetsEXT g_vkCmdSetDescriptorBufferOffsetsEXT;

namespace vk 
{
	struct Queue
	{
		VkQueue handle = VK_NULL_HANDLE;
		uint32_t family = uint32_t(-1);
	};

	enum DeviceQueue : uint32_t
	{
		GRAPHICS = 0,
		PRESENT,
		TRANSFER,
		MAX_QUEUES
	};

	//TODO: make these members private and turn device into a class.
	class Device
	{
	public:
		Device() = default;
		void Init( VkInstance instance, VkSurfaceKHR surface );
		~Device() = default;
		void Destroy();
		Device& operator=(const Device&) = delete;
		Device& operator=(Device&&) = delete;

		//functionality
		[[nodiscard]] uint32_t GetMemoryType( uint32_t typeBits, VkMemoryPropertyFlags flags ) const;
		[[nodiscard]] VkPhysicalDeviceDescriptorBufferPropertiesEXT GetDescriptorBufferProperties() const;
		[[nodiscard]] VkPhysicalDeviceProperties GetProperties() const;

		const VkDevice GetDevice() const;
		const VkPhysicalDevice GetGPU() const;

		const vk::Queue& GetQueue( DeviceQueue queue ) const;

		Buffer CreateBuffer( size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data );
		void AllocateCommandBuffers( VkCommandBuffer* commandBuffers, uint32_t count ) const;
		void FreeCommandBuffers( const VkCommandBuffer* commandBuffers, uint32_t count ) const;
		VkCommandBuffer CreateCommandBuffer( VkCommandBufferLevel level, bool begin );
		void FlushCommandBuffer( VkCommandBuffer cmdBuffer, VkQueue queue, bool free );
		void AddExtension(const char* name);
	//helpers
	private:
		void FindPhysicalDevices( VkInstance instance );
		void FindQueueFamilies( VkSurfaceKHR windowSurface );
		void InitializeLogicalDevice();
		void CheckRequestedExtensions() const;
	private:
		std::vector<const char*> m_requestedExtensions;
		std::array<vk::Queue, DeviceQueue::MAX_QUEUES> m_queues;

		VkPhysicalDeviceDescriptorBufferPropertiesEXT m_descriptorBufferProps = {};
		VkPhysicalDeviceMemoryProperties m_memoryProps = {};
		VkPhysicalDeviceProperties m_properties = {};

		VkPhysicalDevice m_gpu = VK_NULL_HANDLE;
		VkDevice m_device      = VK_NULL_HANDLE;

		VkCommandPool m_commandPool = VK_NULL_HANDLE;

	};

}

