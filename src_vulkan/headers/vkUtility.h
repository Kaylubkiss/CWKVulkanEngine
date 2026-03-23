#pragma once

//this should be made to do something more useful, but for now it can be a useful alias
#define VK_CHECK_RESULT(function) assert(function == VK_SUCCESS)

namespace vk 
{
	namespace util 
	{

		inline VkDeviceSize AlignedSize(VkDeviceSize size, VkDeviceSize alignment)
		{
			return (size + alignment - 1) & ~(alignment - 1);
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo();

		static void check_vk_result(VkResult err)
		{
			if (err == 0)
				return;
			//fun fact: fprintf will return the length of the string it outputted..
			//https://stackoverflow.com/questions/29931016/return-value-of-fprintf
			int result = fprintf(stderr, "[Vulkan] Error: VkResult = %d\n", err);

			if (err < 0)
				abort();
		}

		VkFormat findSupportedFormat(const VkPhysicalDevice p_device, const std::vector<VkFormat>& possibleFormats,
			VkImageTiling tiling, VkFormatFeatureFlags features);

		bool FormatIsSupported(const VkPhysicalDevice p_device, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features);

		//BEGIN TODO: move this into a device-specific class.
		//use command pool
		void TransitionImageLayout(const VkDevice l_device, const VkCommandPool cmdPool, 
			const VkQueue& gfxQueue, uint32_t srcQueue, uint32_t dstQueue,
			VkImage image, VkFormat format, 
			VkImageLayout oldLayout, VkImageLayout newLayout, 
			uint32_t mipLevels);

		//use command pool
		void copyBufferToImage(const VkDevice l_device, const VkCommandPool cmdPool, VkBuffer buffer, const VkQueue gfxQueue, VkImage image, uint32_t width, uint32_t height);
		//use command buffer
		void copyBufferToImage(const VkDevice l_device, const VkCommandBuffer cmdBuffer, VkBuffer buffer, const VkQueue gfxQueue, VkImage image, uint32_t width, uint32_t height);

		void GenerateMipMaps(const VkPhysicalDevice p_device, const VkDevice l_device, const VkCommandPool& cmdPool, const VkQueue gfxQueue, VkImage image, VkFormat imgFormat, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels);
		//...END OF TODO

		bool FormatIsFilterable( const VkPhysicalDevice p_device, VkFormat format, VkImageTiling tiling );

		uint32_t CalculateMipLevels ( const uint32_t& imageWidth, const uint32_t& imageHeight );

		std::optional<std::string> ReadFile( const std::string& filename );

		bool CheckInstanceLayerSupport( const char* layers[], int layersSize );

		bool CheckInstanceExtensionSupport( const char* extensions[], int extension_count );
	}
}