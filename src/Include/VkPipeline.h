/*
	* filename: VkPipeline.h
	* author: Caleb Kissinger
*/
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <list>
#include "shaderc/shaderc.hpp"

namespace vk 
{
	/*
		*@brief struct used to compile and track a particular shader file 
	*/
	struct ShaderModuleInfo
	{
		std::string filename; /*used by hot reloader to detect file status */
		shaderc_shader_kind shaderc_kind; /*arguments in runtime shader compilation */
		VkShaderModule handle = VK_NULL_HANDLE;
		VkShaderStageFlagBits flags;
	};

	/*
		*@brief describes the process to a renderpass, containing a series of shaders
		and rendering information. 
	*/
	class Pipeline 
	{
		VkPipeline handle = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE; 
		VkPrimitiveTopology mTopology = {};

		std::vector<ShaderModuleInfo> shaderModules;

		public:
			/*
				*@brief Compiles a shader file to sprv, creates a shader module and puts it into a vector.
				*@param l_device: logical device associated with the application's vulkan instance.
				*@param filename: name to the uncompiled shader source.
				*@param shaderFlags: specifies what shader stage the source is working in.
				*@param shader_kind: shaderc's shader stage flag, used for compiling to sprv. 
				Defaults to shaderc_vertex_shader.
			*/
			void AddModule(const VkDevice l_device, const std::string& filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shader_kind = shaderc_vertex_shader);

			/*
				*@brief Creates the pipeline handle with the user provided shader modules.
				*@param l_device: logical device associated with the application's vulkan instance.
				*@param renderPass: the thing rendering into the framebuffer after shader processing, argument needed for pipeline creation.
				*@param topology: the geometric shape of a rendered object.
			*/
			void Finalize(const VkDevice l_device, const VkRenderPass renderPass, VkPrimitiveTopology topology);

			/*
				*@brief Destroys the pipeline handle and all the vulkan objects (shaders, descriptory layout) created under it.
				*@param l_device: logical device associated with the application's vulkan instance.
			*/
			void Destroy(const VkDevice l_device);

			/*
				*@brief Destroys the current pipeline handle, and creates a new one. 
				*@param l_device: logical device associated with the application's vulkan instance.
			*/
			void Recreate(const VkDevice l_device, const VkRenderPass renderPass);
			
			/* getters */

			const VkDescriptorSetLayout DescriptorSetLayout() const;
			const VkPipeline Handle() const;
			const VkPipelineLayout Layout() const;
			std::vector<ShaderModuleInfo>& ShaderModules();

	};

}
