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

			HotReloader& operator=(const HotReloader& other);
			HotReloader(VkDevice* l_device, vk::Pipeline& pipeline, VkRenderPass* renderPass);
			void HotReload();

		private:
			void AddPipeline(vk::Pipeline& pipeline);
			
	};


}