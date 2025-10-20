#pragma once
#include "vkContextBase.h"

namespace vk 
{
	#define LIGHT_COUNT 2

	class DeferredContext : public ContextBase 
	{
		enum DeferredPipelines
		{
			LIGHTPASS = 0,
			MRT
		};

		enum DeferredRenderTargets 
		{
			RT_POSITION = 0,
			RT_NORMAL,
			RT_ALBEDO
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
			vk::Buffer deferredShadow;
			vk::Buffer deferredLightPass;
		} uniformBuffers{};

		VkDescriptorSetLayout sceneDescriptorSetLayout = VK_NULL_HANDLE;

		//NOTE: this will all be done offscreen because we have a main renderpass from the swapchain we'll 
		//read the results of this from
		Framebuffer deferredPassFB;

		struct 
		{
			VkDescriptorSet deferred;
			VkDescriptorSet composition;
		} descriptorSets{};

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
		void IntializeDeferredFramebuffer();
		void InitializeUniforms();
		void UpdateScreenUniforms();
		void UpdateSceneUniforms();

	};

}