#ifndef VK_BUFFER_HPP
#define VK_BUFFER_HPP

namespace vk 
{
	class Device;

	class Buffer final
	{
	public:
		//assume that build info is shared among all buffers.
		Buffer() = default;
		Buffer( const Buffer& other ) = delete;
		Buffer( Buffer&& other ) noexcept;
		Buffer( const vk::Device* devicePtr, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags,
			size_t size, void* data = nullptr );

		Buffer& operator=( const Buffer& other ) = delete;
		Buffer& operator=( Buffer&& other ) noexcept;

		~Buffer();

		//the actual destructor method, helps with default assignment operator not causing errors
		[[nodiscard]] uint64_t GetDeviceAddress() const;
		[[nodiscard]] void* GetMappedMemory() const;
		[[nodiscard]] VkDescriptorBufferInfo GetDescriptor() const;
		[[nodiscard]] VkBuffer GetHandle() const;
		[[nodiscard]] VkDeviceSize GetSize() const;

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

#endif


