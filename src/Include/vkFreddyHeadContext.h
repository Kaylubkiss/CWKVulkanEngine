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

			//descriptors...
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

		public:
			FreddyHeadScene();
			~FreddyHeadScene();

			virtual void RecordCommandBuffers(vk::ObjectManager& objManager) override;
			virtual std::vector<VkWriteDescriptorSet> WriteDescriptorBuffers(VkDescriptorSet descriptorSet) override;
			virtual void InitializeScene(ObjectManager& objManager) override;
			virtual uint32_t SamplerDescriptorSetBinding() override;
			virtual const VkDescriptorSetLayout DescriptorSetLayout() const override;
			virtual void Render();

		protected:
			virtual void InitializePipeline(std::string vsFile, std::string fsFile) override;
			virtual void InitializeDescriptors() override;

		private:
			void UpdateUniforms();
	};


}