#pragma once

#include "vkContextBase.h"
#include "vkGlobal.h"

namespace vk 
{
	class FreddyHeadScene : public ContextBase
	{
		private:

			//uniform(s)		
			struct UniformData {
				uTransformObject transform;
				glm::vec3 camPos = glm::vec3(0.0);
				uLightObject light;
			} sceneUniformData;

			vk::Buffer sceneUniformBuffer;

			//descriptors...
			Texture defaultTexture;

			VkDescriptorSet sceneDescriptorSet = VK_NULL_HANDLE;
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