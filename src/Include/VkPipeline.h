#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#ifdef _DEBUG
#include "shaderc/shaderc.hpp"
#endif

namespace vk 
{
	struct ShaderModuleInfo
	{
		#ifdef _DEBUG
			std::string filepath; //for hot reloader...
			shaderc_shader_kind shaderc_kind;
		#endif
		VkShaderModule handle;
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

#ifdef _DEBUG
			void AddModule(const VkDevice l_device, const std::string& filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shader_kind = shaderc_vertex_shader);
#else
			void AddModule(const VkDevice l_device, const std::string& filename, VkShaderStageFlagBits shaderFlags);
#endif
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
