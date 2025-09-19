#pragma once
#include "vkContextBase.h"

namespace vk 
{
	class DeferredContext : public ContextBase 
	{

		

		struct UniformDataDeferred 
		{
			uTransformObject uTransform;
		} uniformDataDeferred{};

		struct
		{
			vk::Buffer deferred;
		} uniformBuffers{};

		VkDescriptorSetLayout sceneDescriptorSetLayout = VK_NULL_HANDLE;

		//NOTE: this will all be done offscreen because we have a main renderpass from the swapchain we'll 
		//read the results of this from
		struct Framebuffer {
			int32_t width = 0;
			int32_t height = 0;
			VkFramebuffer framebuffer = VK_NULL_HANDLE;
			FramebufferAttachment position, normal, albedo;
			FramebufferAttachment depth;
			VkRenderPass renderPass = VK_NULL_HANDLE;
		} deferredPass;

		struct 
		{
			VkDescriptorSet deferred;
			VkDescriptorSet composition;
		} descriptorSets{};


		VkSampler colorSampler = VK_NULL_HANDLE;

		VkPipeline deferredMRTPipeline = VK_NULL_HANDLE;

	public:
		DeferredContext();
		~DeferredContext();

		virtual void RecordCommandBuffers(vk::ObjectManager& objManager) override;
		virtual void InitializeScene(ObjectManager& objManager) override;
		virtual std::vector<VkWriteDescriptorSet> WriteDescriptorBuffers(VkDescriptorSet descriptorSet) override;
		virtual uint32_t SamplerDescriptorSetBinding() override;
		virtual const VkDescriptorSetLayout DescriptorSetLayout() const override;
		void ResizeWindow() override;

		virtual void Render() override;

	protected:
		virtual void InitializePipeline(std::string vsFile = "", std::string fsFile = "") override;
		virtual void InitializeDescriptors() override;
	


	private:
		void InitializeDeferredRenderPass();
		void IntializeDeferredFramebuffer();
		void IntializeColorSampler();
		void InitializeUniforms();
		void UpdateUniforms();

	};

}