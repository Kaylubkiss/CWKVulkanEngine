#pragma once


#include "shaderc/shaderc.hpp"
#include <sys/stat.h>
#include <vkPipeline.h>

namespace vk 
{
	class HotReloader
	{
		VkDevice* appDevicePtr = nullptr;
		VkRenderPass* renderPassPtr = nullptr;

		struct ShaderFileInfo
		{
			vk::ShaderModuleInfo* moduleInfo;
			time_t last_modification = 0;
		};

		vk::Pipeline* pipelinePtr = nullptr;

		std::vector<ShaderFileInfo> shaderInfos;

		
		public:
			HotReloader() = default;
			~HotReloader() = default;

			HotReloader(VkDevice* l_device, vk::Pipeline* pipeline, VkRenderPass* renderPass);
			void HotReload();

		private:
			void AddPipeline(vk::Pipeline* pipeline);
			
	};


}