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
#include "vkWindow.h"

namespace vk 
{
	/*
		*@brief struct used to compile and track a particular shader file 
	*/
	struct ShaderModuleInfo
	{
		std::string mFilePath;
		std::string mFileName; /*used by hot reloader to detect file status */
		VkShaderModule mHandle = VK_NULL_HANDLE;
		VkShaderStageFlagBits mFlags;

		shaderc_shader_kind mShaderKind; /*arguments in runtime shader compilation */

		/*
			*@brief intializer list for a ShaderModuleInfo object. Compiles the specified shader source to sprv and also sets the shader stage flags.
			
			*@param l_device: logical device associated with the application's vulkan instance.
			*@param filename: the name of the shader source.
			*@param shaderFlags: specifies what shader stage the source file is working in.
			*@param shaderc_kind: similar to shaderFlags, argument needed for shader compilation to sprv.
		*/
		ShaderModuleInfo(const VkDevice l_device, std::string filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shaderc_kind = shaderc_vertex_shader);


	};

	/*
		*@brief describes the process to a renderpass, containing a series of shaders
		and rendering information. 
	*/
	struct Pipeline 
	{
		VkPipeline handle = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE; 
		VkPrimitiveTopology mTopology = {};
		
		//TODO: move renderpass out of this class.
		VkRenderPass mRenderPass = VK_NULL_HANDLE;

		std::vector<ShaderModuleInfo> shaderModules;

		/*
			*@brief Compiles a shader file to sprv, creates a shader module and puts it into a vector.
			
			*@param shaderModuleInfo: added to list of module infos, which describe the shader source a part of the pipeline.
			
			*@return pointer to this, which allows chaining of this method on one line.
		*/
		vk::Pipeline& AddModule(const ShaderModuleInfo& shaderModuleInfo);

		vk::Pipeline& AddPipelineLayout(const VkPipelineLayout& pipelineLayout);

		vk::Pipeline& AddRenderPass(const VkRenderPass& renderPass);


		/*void InitRenderDepthInformation(const VkDevice l_device, const VkFormat& depthFormat);

		void InitRenderPass(const VkDevice l_device, const VkFormat& depthFormat);*/

		/*
			*@brief Creates the pipeline handle with the user provided shader modules.
			*
			*@param l_device: logical device associated with the application's vulkan instance.
			*@param renderPass: the thing rendering into the framebuffer after shader processing, argument needed for pipeline creation.
			*@param topology: the geometric shape of a rendered object.
			*
			* @return this pointer to pipeline, for chaining commands on one line.
		*/
		vk::Pipeline& Finalize(const VkDevice l_device, const VkPhysicalDevice p_device, const vk::Window& appWindow, VkPrimitiveTopology topology);

		

		/*
			*@brief Destroys the pipeline handle and all the vulkan objects (shaders, descriptory layout) created under it.
			
			*@param l_device: logical device associated with the application's vulkan instance.
		*/
		void Destroy(const VkDevice l_device);

		/*
			*@brief Destroys the current pipeline handle, and creates a new one. 
			
			*@param l_device: logical device associated with the application's vulkan instance.
		*/
		void Recreate(const VkDevice l_device);
		
		/* getters */

		const VkPipeline Handle() const;
		const VkPipelineLayout Layout() const;
		vk::ShaderModuleInfo& ShaderModule(size_t index);
		const std::vector<vk::ShaderModuleInfo>& ShaderModules() const;
		VkRenderPass RenderPass();

	};

}
