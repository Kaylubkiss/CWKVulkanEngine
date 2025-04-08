#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace vk 
{
	struct Pipeline 
	{
		//pipeline information
		VkPipeline handle = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;

		struct ShaderModuleInfo{

			VkShaderModule module;
			VkShaderStageFlagBits flags;
		};
		std::vector<ShaderModuleInfo> shaderModules;



		void AddModule(const VkDevice l_device, const std::string& filename, VkShaderStageFlagBits type, shaderc_shader_kind shader_kind = shaderc_vertex_shader);

		void Finalize(const VkDevice l_device, const VkRenderPass renderPass, VkPrimitiveTopology topology);

		void Destroy(const VkDevice l_device);
	};

}
