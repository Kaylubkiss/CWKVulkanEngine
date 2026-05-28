#include "vkBuffer.h"
#include "vkInit.h"

namespace vk 
{
	Buffer::Buffer( Buffer&& other ) noexcept
	{
		if (this != &other)
		{
			this->c_device = other.c_device;
			this->m_size = other.m_size;
			this->m_memory = other.m_memory;
			this->m_handle = other.m_handle;
			this->m_mappedMemory = other.m_mappedMemory;
			this->m_descriptor = other.m_descriptor;

			other.c_device = VK_NULL_HANDLE;
			other.m_handle = VK_NULL_HANDLE;
			other.m_memory = VK_NULL_HANDLE;
			other.m_mappedMemory = VK_NULL_HANDLE;
			other.m_descriptor = {};
			other.m_size = 0;
		}
	}

	Buffer& Buffer::operator=( Buffer&& other ) noexcept
	{
		if (this != &other)
		{
			this->c_device = other.c_device;
			this->m_size = other.m_size;
			this->m_memory = other.m_memory;
			this->m_handle = other.m_handle;
			this->m_mappedMemory = other.m_mappedMemory;
			this->m_descriptor = other.m_descriptor;

			other.c_device = VK_NULL_HANDLE;
			other.m_handle = VK_NULL_HANDLE;
			other.m_memory = VK_NULL_HANDLE;
			other.m_mappedMemory = VK_NULL_HANDLE;
			other.m_descriptor = {};
			other.m_size = 0;
		}

		return *this;
	}

	Buffer::Buffer( const vk::Device* devicePtr, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags,
		size_t size, void* data )
	{
		c_device = devicePtr->GetDevice();
		m_size = static_cast<VkDeviceSize>(size);

		VkBufferCreateInfo bufferCreateInfo = vk::init::BufferCreateInfo(usage, m_size);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// can only use CONCURRENT if .queueFamilyIndexCount > 0

		VK_CHECK_RESULT(vkCreateBuffer(c_device, &bufferCreateInfo, nullptr, &m_handle));

		VkMemoryRequirements			memoryRequirments;
		vkGetBufferMemoryRequirements(c_device, m_handle, &memoryRequirments);

		VkMemoryAllocateInfo vmai = vk::init::MemoryAllocateInfo();
		vmai.allocationSize = memoryRequirments.size;

		VkPhysicalDeviceMemoryProperties	vpdmp;
		vkGetPhysicalDeviceMemoryProperties(devicePtr->GetGPU(), &vpdmp);

		vmai.memoryTypeIndex = devicePtr->GetMemoryType(memoryRequirments.memoryTypeBits, flags);

		VK_CHECK_RESULT(vkAllocateMemory(c_device, &vmai, nullptr, &m_memory));
		VK_CHECK_RESULT(vkBindBufferMemory(c_device, m_handle, m_memory, 0));

		if (data != nullptr)
		{
			Buffer::Map();

			if (m_mappedMemory != nullptr)
			{
				memcpy(m_mappedMemory, data, m_size);
			}

			if ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				Buffer::Flush();
			}


			Buffer::UnMap();
		}

		Buffer::SetDescriptor(m_size, 0);
	}

	Buffer::~Buffer()
	{
		if (c_device != VK_NULL_HANDLE)
		{
			if (m_memory != VK_NULL_HANDLE)
			{
				UnMap(); //already checks if the mapped memory is null before freeing.
				vkFreeMemory(c_device, m_memory, nullptr);
				m_memory = VK_NULL_HANDLE;
			}

			if (m_handle != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(c_device, m_handle, nullptr);
				m_handle = VK_NULL_HANDLE;
			}
		}
	}

	VkDeviceAddress Buffer::GetDeviceAddress() const
	{
		VkBufferDeviceAddressInfo bufferAddress = {};
		bufferAddress.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferAddress.buffer = m_handle;
		return vkGetBufferDeviceAddress(c_device, &bufferAddress);
	}

	void* Buffer::GetMappedMemory() const 
	{
		assert(m_mappedMemory != nullptr);
		return m_mappedMemory;
	}

	VkDescriptorBufferInfo Buffer::GetDescriptor() const 
	{
		return m_descriptor;
	}

	VkBuffer Buffer::GetHandle() const
	{
		return m_handle;
	}

	VkDeviceSize Buffer::GetSize() const 
	{
		return m_size;
	}

	void Buffer::SetDescriptor(VkDeviceSize size, VkDeviceSize offset) 
	{
		m_descriptor.buffer = m_handle;
		m_descriptor.range  = size;
		m_descriptor.offset = offset;
	}

	void Buffer::Map() 
	{
		if (m_mappedMemory == nullptr) 
		{
			VK_CHECK_RESULT(vkMapMemory(c_device, m_memory, 0, m_size, 0, &m_mappedMemory));
		}
	}

	void Buffer::Flush() 
	{
		VkMappedMemoryRange mappedRange = vk::init::MappedMemoryRange();
		mappedRange.memory = m_memory;
		mappedRange.offset = 0;
		mappedRange.size = m_size;
		vkFlushMappedMemoryRanges(c_device, 1, &mappedRange);
	}

	void Buffer::UnMap() 
	{
		if (m_mappedMemory != nullptr) 
		{
			vkUnmapMemory(c_device, m_memory);
			m_mappedMemory = nullptr;
		}

	}
}