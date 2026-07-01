#ifndef VK_UTIL_HPP
#define VK_UTIL_HPP

//this should be made to do something more useful, but for now it can be a useful alias
#define VK_CHECK_RESULT(function) \
{ \
	VkResult result = function; \
	if (result != VK_SUCCESS) \
	{ \
		std::cerr << "VkResult " << result << "at " << __FILE__ << " on line " << __LINE__ << '\n'; \
		assert(result == VK_SUCCESS); \
	} \
}\

namespace vk 
{
	namespace util 
	{

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

		void RecordImageLayoutTransition( VkCommandBuffer cmdBuffer, VkImage image,
			uint32_t srcQueue, uint32_t dstQueue,
			VkImageLayout oldLayout, VkImageLayout newLayout );

		void SubmitCommandToQueue( VkDevice device, VkCommandBuffer cmdBuffer, VkQueue queue, VkFence fence, std::optional<std::mutex> submissionMutex );

		void RecordBlitMipMapImages( VkCommandBuffer cmdBuffer, VkImage image,
			uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels, uint32_t layerCount );

		bool FormatIsFilterable( const VkPhysicalDevice p_device, VkFormat format, VkImageTiling tiling );

		uint32_t CalculateMipLevels ( uint32_t imageWidth, uint32_t imageHeight );

		std::optional<std::string> ReadFile( const std::string& filename );

		bool CheckInstanceLayerSupport( const char* layers[], int layersSize );

		bool CheckInstanceExtensionSupport( const char* extensions[], int extension_count );

		VkCommandBuffer beginSingleTimeCommand( const VkDevice l_device, const VkCommandPool cmdPool );

		void endSingleTimeCommand( const VkDevice l_device, VkCommandBuffer commandBuffer,
			const VkCommandPool cmdPool, const VkQueue gfxQueue );

		VkImage CreateImage ( const vk::Device* devicePtr, const VkImageCreateInfo& createInfo,
			VkDeviceMemory& imageMemory, VkMemoryPropertyFlags memoryFlags );

		inline VkDeviceSize AlignedSize(VkDeviceSize size, VkDeviceSize alignment)
		{
			return (size + alignment - 1) & ~(alignment - 1);
		}


	}
}

#endif