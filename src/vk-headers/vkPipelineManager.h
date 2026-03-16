/*
	* filename: VkPipelineManager.h
	* author: Caleb Kissinger
*/
#pragma once
#include "vkShaderModule.h"

//PipelineManager
namespace vk 
{
	struct Pipeline 
	{
		std::function<void()> createFunc = nullptr;
		std::vector<ShaderModuleInfo> shaderModules;
		VkPipeline handle = VK_NULL_HANDLE;
	};

	struct HotReloadModuleInfo
	{
		size_t index = 0;
		std::string path;
	};

	struct HotReloadInfo
	{
		int pipeline_index = -1;
		std::vector<HotReloadModuleInfo> modules;
	};

	/*
		*@brief describes the process to a renderpass, containing a series of shaders
		and rendering information. 
	*/
	class PipelineManager
	{
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

		void AddPipeline( uint32_t pipeline, const VkPipeline handle, std::function<void()>&& createFunc = nullptr );

		void HotReloadShaders();

		void DetectHotReloadableShaders();

		[[nodiscard]] bool HotReloadIsReady() const;

		/* getters */
		VkPipeline Get( uint32_t pipeline );

		const std::vector<ShaderModuleInfo>& GetPipelineShaders( uint32_t pipeline );
	private:
		std::map<uint32_t, vk::Pipeline> pipelines;
		std::map<uint32_t, HotReloadInfo> hotReloadInfos;
		VkDevice contextLogicalDevice = VK_NULL_HANDLE;
	};

}
