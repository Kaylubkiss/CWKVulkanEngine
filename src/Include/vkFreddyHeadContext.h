#pragma once

#include "vkContextBase.h"
#include "vkGlobal.h"

namespace vk 
{
	class FreddyHeadScene : public ContextBase
	{
		private:

			//uniforms
			uTransformObject uTransform;
			vk::Buffer uTransformBuffer;
			
			uLightObject uLight; /* NOTE: weird place to put light! */
			vk::Buffer uLightBuffer;

		public:
			FreddyHeadScene();
			~FreddyHeadScene();

			virtual void RecordCommandBuffers(vk::ObjectManager& objManager) override;
			virtual void ResizeWindow() override;
			virtual std::vector<VkDescriptorBufferInfo> DescriptorBuffers() override;
			virtual void InitializeScene(ObjectManager& objManager) override;

			//unique methods to this class. 
			//	WARNING: you have to cast the pointer of type ContextBase to FreddyHeadScene to use these.
			uTransformObject& GetUniformTransform();
			vk::Buffer& GetUniformTransformBuffer();

		protected:
			virtual void InitializePipeline(std::string vsFile, std::string fsFile) override;
	};


}