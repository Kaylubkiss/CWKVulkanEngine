#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <list>
#include "shaderc/shaderc.hpp"

namespace vk 
{
	struct ShaderModuleInfo
	{
		std::string filename; //for hot reloader...
		shaderc_shader_kind shaderc_kind;
		VkShaderModule handle = VK_NULL_HANDLE;
		VkShaderStageFlagBits flags;

	};

	class Pipeline 
	{
		//pipeline information
		VkPipeline handle = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE; 
		VkPrimitiveTopology mTopology = {};

		std::vector<ShaderModuleInfo> shaderModules;

		public:
			void AddModule(const VkDevice l_device, const std::string& filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shader_kind = shaderc_vertex_shader);

			void Finalize(const VkDevice l_device, const VkRenderPass renderPass, VkPrimitiveTopology topology);

			void Destroy(const VkDevice l_device);

			void Recreate(const VkDevice l_device, const VkRenderPass renderPass);

			//getters
			const VkDescriptorSetLayout DescriptorSetLayout() const;
			const VkPipeline Handle() const;
			const VkPipelineLayout Layout() const;
			std::vector<ShaderModuleInfo>& ShaderModules();

	};

}
