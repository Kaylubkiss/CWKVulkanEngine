/*
	* filename: VkPipelineManager.h
	* author: Caleb Kissinger
*/
#pragma once
#include "shaderc/shaderc.hpp"


//ShaderModuleInfo 
namespace vk 
{
	struct GraphicsContextInfo;

	struct ShaderModuleInfo
	{
		VkShaderModule mHandle = VK_NULL_HANDLE;
		time_t lastModificationTime = 0;
		std::string mFilePath = "";

		VkShaderStageFlagBits mFlags = VK_SHADER_STAGE_ALL;
		shaderc_shader_kind mShaderKind = shaderc_glsl_infer_from_source; /*arguments in runtime shader compilation */

		/*
			*@brief intializer list for a ShaderModuleInfo object. Compiles the specified shader source to sprv and also sets the shader stage flags.

			*@param l_device: logical device associated with the application's vulkan instance.
			*@param filename: the name of the shader source.
			*@param shaderFlags: specifies what shader stage the source file is working in.
			*@param shaderc_kind: similar to shaderFlags, argument needed for shader compilation to sprv.
		*/
		ShaderModuleInfo() = default;
		ShaderModuleInfo(const VkDevice l_device, std::string filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shaderc_kind = shaderc_vertex_shader);


	};
}

//PipelineManager
namespace vk 
{
	struct Pipeline 
	{
		std::function<void()> createFunc = nullptr;
		std::vector<ShaderModuleInfo> shaderModules;
		VkPipeline handle = VK_NULL_HANDLE;
	};

	/*
		*@brief describes the process to a renderpass, containing a series of shaders
		and rendering information. 
	*/
	struct PipelineManager 
	{
		private:
			std::map<uint32_t, vk::Pipeline> pipelines;
			VkDevice contextLogicalDevice = VK_NULL_HANDLE;

		public:

			void Init(std::shared_ptr<GraphicsContextInfo>& contextInfo);


			/*
				*@brief Destroys the pipeline handle and all the vulkan objects (shaders, descriptory layout) created under it.
			*/
			void Destroy();

			/*
				*@brief Compiles a shader file to sprv, creates a shader module and puts it into a vector.
				
				*@param shaderModuleInfo: added to list of module infos, which describe the shader source a part of the pipeline.
				
				*@return void
			*/
			void AddModule( uint32_t pipeline, const ShaderModuleInfo& shaderModuleInfo );

			void AddPipeline( uint32_t pipeline, const VkPipeline handle, std::function<void()> createFunc = nullptr );

			void HotReloadShaders();
			
			/* getters */
			VkPipeline Get( uint32_t pipeline );

			const std::vector<ShaderModuleInfo>& GetPipelineShaders( uint32_t pipeline );

	};

}
