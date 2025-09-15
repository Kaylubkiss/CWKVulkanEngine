#pragma once
#include "vkContextBase.h"

namespace vk 
{
	class DeferredContext : public ContextBase 
	{

		VkDescriptorSetLayout sceneDescriptorSetLayout = VK_NULL_HANDLE;

		//NOTE: this will all be done offscreen because we have a main renderpass from the swapchain we'll 
		//read the results of this from.
		VkRenderPass deferredRenderPass = VK_NULL_HANDLE;

		struct Framebuffer {
			int32_t width, height;

		} deferredFramebuffer;

	public:
		DeferredContext();
		~DeferredContext();

		virtual void RecordCommandBuffers(vk::ObjectManager& objManager) override;
		virtual void InitializeScene(ObjectManager& objManager) override;
		virtual std::vector<VkWriteDescriptorSet> WriteDescriptorBuffers(VkDescriptorSet descriptorSet) override;
		virtual uint32_t SamplerDescriptorSetBinding() override;
		virtual const VkDescriptorSetLayout DescriptorSetLayout() const override;

	protected:
		virtual void InitializePipeline(std::string vsFile = "", std::string fsFile = "") override;
		virtual void InitializeDescriptors() override;


	private:
		void InitializeDeferredRenderPass();

	};

}