/*
	* filename: HotReloader.h
	* author: Caleb Kissinger
*/
#pragma once
#include <sys/stat.h>

namespace vk 
{
	struct HotReloader
	{
		VkDevice appDevicePtr = VK_NULL_HANDLE; /* pointer to application's logical device */
		//vk::Pipeline* pipelinePtr = nullptr;  /* pointer to pipeline with edited shaders.


		/* file status info on shaders from pipeline */
		struct ShaderFileInfo
		{
			size_t module_i = -1; /*If there are multiple pipelinePtrs, might need an index into their respective shaderModule lists. */
			time_t last_modification = 0;
		};
		std::vector<ShaderFileInfo> shaderInfos;  

		
		public:
			HotReloader() = default;
			~HotReloader() = default;

			/* Assigns pipeline pipelinePtr. */
	/*		void AddPipeline(vk::Pipeline& pipeline);*/
			
			
			/* Analyze the assigned files for changes, and then recreate the pipeline
			with the new shaders */
			void HotReload();			
	};


}