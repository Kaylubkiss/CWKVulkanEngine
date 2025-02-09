#include "Common.h"
#include "vkUtility.h"


namespace vk {

	namespace util 
	{
		VkPipelineShaderStageCreateInfo CreateShaderModule(VkDevice logicalDevice, const char* name, VkShaderModule& shaderModule, VkShaderStageFlagBits stage)
		{
			std::ifstream file(name, std::ios::ate | std::ios::binary); //when we initialize, we std::ios::ate points to the end.

			if (!file.is_open())
			{
				throw std::runtime_error("failed to open shader file!");
			}

			char* buffer = nullptr;


			//reads the offset from the beginning of the file
			size_t fileSize = (size_t)file.tellg();

			buffer = new char[fileSize];

			//set the stream to the beginning of the file after being positioned at the end.
			file.seekg(0);

			file.read(buffer, fileSize);

			file.close();


			VkShaderModuleCreateInfo shaderVertModuleInfo =
			{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				nullptr,
				0,
				fileSize,
				reinterpret_cast<const uint32_t*>(buffer)
			};

			VK_CHECK_RESULT(vkCreateShaderModule(logicalDevice, &shaderVertModuleInfo, nullptr, &shaderModule));

			VkPipelineShaderStageCreateInfo shaderStageInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				nullptr,
				0,
				stage,
				shaderModule,
				"main", //entry point -->pName
				nullptr //no specialization constants
			};

			delete[] buffer;

			return shaderStageInfo;

		}

		bool CheckValidationSupport()
		{
			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);

			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


			for (const char* layerName : vulkan_engine::enabledLayerNames)
			{
				bool layerFound = false;
				for (const VkLayerProperties& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (layerFound == false)
				{
					return false;
				}

			}


			return true;

		}

		VkFormat findSupportedFormat(const VkPhysicalDevice p_device, const std::vector<VkFormat>& possibleFormats,
			VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (VkFormat format : possibleFormats)
			{
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(p_device, format, &properties);

				if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
				{
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
				{
					return format;
				}

			}

			throw std::runtime_error("couldn't find a suitable format supported on the physical device.");
		}
	}
}