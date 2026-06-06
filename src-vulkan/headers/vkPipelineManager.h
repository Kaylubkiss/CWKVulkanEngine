/*
	* filename: VkPipelineManager.h
	* author: Caleb Kissinger
*/
#ifndef VK_PIPELINE_MANAGER_HPP
#define VK_PIPELINE_MANAGER_HPP

#include "vkShaderModule.h"
#include "vkPipelineBuilder.h"

//PipelineManager
namespace vk 
{
	struct Pipeline 
	{
		std::unique_ptr<PipelineBuilder> pipelineBuilder;
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
		std::vector<HotReloadModuleInfo> moduleInfos;
	};

	/*
		*@brief describes the process to a renderpass, containing a series of shaders
		and rendering information. 
	*/
	class PipelineManager
	{
	public:
		PipelineManager() = default;
		PipelineManager( const GraphicsContextInfo& contextInfo );

		PipelineManager( const PipelineManager& other ) = delete;
		PipelineManager& operator=( const PipelineManager& other ) = delete;

		PipelineManager( PipelineManager&& other ) noexcept;
		PipelineManager& operator=( PipelineManager&& other ) noexcept;


		/*
			*@brief Destroys the pipeline handle and all the vulkan objects (shaders, descriptory layout) created under it.
		*/
		~PipelineManager();
		/*
			*@brief Compiles a shader file to sprv, creates a shader module and puts it into a vector.

			*@param shaderModuleInfo: added to list of module infos, which describe the shader source a part of the pipeline.

			*@return void
		*/

		void AddPipeline( uint32_t pipeline, Pipeline& pipelineInfo );

		void HotReloadShaders();

		void DetectHotReloadableShaders();

		[[nodiscard]] bool HotReloadIsReady() const;

		VkPipeline Get( uint32_t pipeline );

	private:
		std::map<uint32_t, vk::Pipeline> pipelines;
		std::map<uint32_t, HotReloadInfo> hotReloadInfos;
		VkDevice contextLogicalDevice = VK_NULL_HANDLE;
	};

}

#endif
