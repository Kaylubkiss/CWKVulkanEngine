#pragma once
#include "vkContextBase.h"

namespace vk 
{
	class DeferredContext : public ContextBase 
	{
		enum DeferredPipelines
		{
			LIGHTPASS = 0,
			MRT
		};

		struct UniformDataMRT
		{
			uTransformObject uTransform;
		} uniformDataMRT{};


		struct UniformDataLightPass {
			uLightObject light;
			glm::vec3 viewPosition;
		} uniformDataLightPass {};

		struct
		{
			vk::Buffer deferredMRT;
			vk::Buffer deferredLightPass;
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


		VkSampler colorSampler = VK_NULL_HANDLE; //for the attachments created at the end of MRT pass

		Texture defaultTexture;

	public:
		DeferredContext();
		~DeferredContext();

		virtual void RecordCommandBuffers() override;
		virtual void UpdateUI() override;
		virtual void InitializeScene(ObjectManager& objManager) override;
		virtual void ResizeWindowDerived() override;

		virtual void Render() override;

	protected:
		virtual void InitializePipeline(std::string vsFile = "", std::string fsFile = "") override;
		virtual void InitializeDescriptors() override;
		virtual void FillOutGraphicsContextInfo() override;
	


	private:
		void InitializeDeferredRenderPass();
		void IntializeDeferredFramebuffer();
		void IntializeColorSampler();
		void InitializeUniforms();
		void UpdateScreenUniforms();
		void UpdateSceneUniforms();

	};

}