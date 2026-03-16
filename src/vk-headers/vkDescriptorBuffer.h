#pragma once

extern PFN_vkGetDescriptorSetLayoutBindingOffsetEXT g_vkGetDescriptorSetLayoutBindingOffsetEXT;
extern PFN_vkGetDescriptorSetLayoutSizeEXT g_vkGetDescriptorSetLayoutSizeEXT;
extern PFN_vkGetDescriptorEXT g_vkGetDescriptorEXT;
extern PFN_vkCmdBindDescriptorBuffersEXT g_vkCmdBindDescriptorBuffersEXT;
extern PFN_vkCmdSetDescriptorBufferOffsetsEXT g_vkCmdSetDescriptorBufferOffsetsEXT;

namespace vk
{
	//row = frame
	//col = binding
	typedef std::vector<std::vector<const vk::Buffer*>> resourceBufferPtrs2D;
	typedef std::vector<std::vector<VkDescriptorImageInfo>> imageBuffers2D;

    struct DescriptorBufferCreateInfo
	{
		uint32_t minTextureCount = 0;
		vk::Device* devicePtr;
		VkDescriptorSetLayoutBinding* pLayoutBindings = nullptr;
    	uint32_t layoutBindingCount = 0;
		resourceBufferPtrs2D resourceDescriptorData;
    	imageBuffers2D imageDescriptorData;
		VkBufferUsageFlags bufferUsageFlags;
		VkMemoryPropertyFlags memoryProperties;
	};

	class DescriptorBuffer
	{
	public:
		DescriptorBuffer() = default;
		~DescriptorBuffer() = default;

		[[nodiscard]] const vk::Buffer& GetBuffer() const;
		[[nodiscard]] const std::vector<VkDeviceSize>& GetBindingOffsets() const;
		[[nodiscard]] VkDeviceSize GetLayoutSize() const;
		[[nodiscard]] VkDescriptorSetLayout GetLayout() const;

		void Destroy();
		void Create( const vk::DescriptorBufferCreateInfo& createInfo );
	private:
		void FillResourceDescriptorBuffers( const vk::DescriptorBufferCreateInfo& createInfo );
		void FillImageDescriptorBuffers( const vk::DescriptorBufferCreateInfo& createInfo );
		static void GetDescriptorLayoutSize( const vk::Device* device, VkDescriptorSetLayout layout, VkDeviceSize* size );
		static void GetDescriptorLayoutBindingOffsets( const vk::Device* device, VkDescriptorSetLayout layout,
		VkDeviceSize offsets[], uint32_t binding_count );
	private:
		//per-frame resources need to be independently updated according to which frame is active.
		//1st index of "buffers" is the minimum ever used by a descriptor buffer.
		vk::Buffer m_buffer;

		//at least 1 binding (binding 0)
		std::vector<VkDeviceSize> m_bindingOffsets = { 0ull };

		VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
		VkDeviceSize m_setLayoutSize = 0ull;

		vk::Device* m_devicePtr = nullptr;
	};

}