#pragma once

#include "vkContextBase.h"
#include "vkGlobal.h"

namespace vk 
{
	class FreddyHeadScene : public ContextBase
	{
		private:

			//uniform(s)		
			uLightObject uLight; /* NOTE: weird place to put light! */
			vk::Buffer uLightBuffer;

		public:
			FreddyHeadScene();
			~FreddyHeadScene();

			virtual void RecordCommandBuffers(vk::ObjectManager& objManager) override;
			virtual void ResizeWindow() override;
			virtual std::vector<VkDescriptorBufferInfo> DescriptorBuffers() override;
			virtual void InitializeScene(ObjectManager& objManager) override;

		protected:
			virtual void InitializePipeline(std::string vsFile, std::string fsFile) override;
			virtual void InitializeDescriptorPool() override ;
	};


}