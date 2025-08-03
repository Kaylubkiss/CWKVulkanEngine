#pragma once

#include "vkContextBase.h"
#include "vkGlobal.h"
#include "vkResource.h"

namespace vk 
{
	class ShadowMapScene : public ContextBase
	{
		private:


			float zNear = 1.f;
			float zFar = 96.f;

			float lightFOV = 45.f;

			float depthBiasConstant = 1.25f;
			float depthBiasSlope = 1.75f;

			//uniform(s)		
			struct UniformDataScene 
			{
				uTransformObject transform = {};
				glm::mat4 depthBiasMVP = glm::mat4(0.f);
				uLightObject light = {};

				float zNear = 0.f;
				float zFar = 0.f;
			} uniformDataScene{};

			struct UniformDataOffscreen 
			{
				glm::mat4 depthMVP = glm::mat4(0.f);
			} uniformDataOffscreen{};

			struct {
				vk::Buffer scene;
				vk::Buffer offscreen;
			} uniformBuffers{};

			struct {
				uint32_t width, height = 0;
				VkFramebuffer frameBuffer = VK_NULL_HANDLE;
				rsc::DepthStencil depth;
				VkRenderPass renderPass = VK_NULL_HANDLE;
				VkSampler depthSampler = VK_NULL_HANDLE;
			} offscreenPass{};


			//pipeline for scene here...
			VkPipeline offscreenPipeline = VK_NULL_HANDLE;
			
			VkDescriptorSetLayout sceneDescriptorLayout = VK_NULL_HANDLE;
			
			struct {
				VkDescriptorSet offscreen;
				VkDescriptorSet scene;
			} descriptorSets{};

			const uint32_t shadowMapSize = 2048;

		public:
			ShadowMapScene();
			~ShadowMapScene();

			virtual void RecordCommandBuffers(vk::ObjectManager& objManager) override;
			virtual void ResizeWindow() override;
			virtual void InitializeScene(ObjectManager& objManager) override;

			virtual void Render();

		protected:
			virtual void InitializePipeline(std::string vsFile, std::string fsFile) override;
			virtual void InitializeDescriptors() override;

			//class-specific methods.
		private:
			void InitializeOffscreenFramebuffer();
			void InitializeOffscreenRenderPass();
			void UpdateSceneUniforms();
			void UpdateOffscreenUniforms();
	};


}