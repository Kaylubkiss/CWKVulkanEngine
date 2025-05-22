#pragma once



#include <sys/stat.h>
#include <vkPipeline.h>

namespace vk 
{
	class HotReloader
	{

		
		VkDevice appDevicePtr = VK_NULL_HANDLE;
		VkRenderPass renderPassPtr = VK_NULL_HANDLE;

		vk::Pipeline* pipelinePtr = nullptr;


		struct ShaderFileInfo
		{
			size_t module_i = -1;
			time_t last_modification = 0;
		};
		std::vector<ShaderFileInfo> shaderInfos;

		
		public:
			HotReloader() = default;
			~HotReloader() = default;

			/*
				* @brief Initializes the hot reloader object.
				* @param l_device: logical device which owns the pipeline associated.
				* @param pipeline: the pipeline which has the shader stages associated.
				* @param renderPass: information that describes a frame in the pipeline.
			*/
			HotReloader(VkDevice* l_device, vk::Pipeline& pipeline, VkRenderPass* renderPass);
			
			/*
				* @brief Called every frame to check the file status of a shader, and recreates
				* the pipeline's shader modules if any change occurs.
			*/
			void HotReload();

		private:
			/*
				* @brief Adds a pipeline which the hot reloader can edit if changes occur
				in its associated shaders.
				* @param pipeline: the pipeline which has the shader stages associated.
			*/
			void AddPipeline(vk::Pipeline& pipeline);
			
	};


}