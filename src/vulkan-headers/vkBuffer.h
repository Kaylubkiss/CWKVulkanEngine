#pragma once

namespace vk 
{
	class Device;

	class Buffer final
	{
	public:
		//assume that build info is shared among all buffers.
		Buffer() = default;
		Buffer( const vk::Device* devicePtr, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, 
			size_t size, void* data = nullptr );
		~Buffer() = default;
		//the actual destructor method, helps with default assignment operator not causing errors.
		void Destroy();
		
		uint64_t GetDeviceAddress() const;		
		void* GetMappedMemory() const;
		VkDescriptorBufferInfo GetDescriptor() const;
		VkBuffer GetHandle() const;
		VkDeviceSize GetSize() const;

		void Map();
		void Flush();
		void UnMap();
	private:
		void SetDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	private:
		VkDevice c_device       = VK_NULL_HANDLE;
		VkBuffer m_handle       = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkDeviceSize m_size     = 0;
		void* m_mappedMemory    = nullptr;

		VkDescriptorBufferInfo m_descriptor = {};

	};
}


