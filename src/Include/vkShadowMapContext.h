#pragma once

#include "vkContextBase.h"
#include "vkGlobal.h"

namespace vk 
{
	class ShadowMapScene : public ContextBase
	{
		private:

			bool showDebug = false;
			float zNear = 1.0f;
			float zFar = 96.f;

			float lightFOV = 45.f;

			float depthBiasConstant = 1.25f;
			float depthBiasSlope = 1.75f;

			//uniform(s)		
			struct UniformDataScene 
			{
				uTransformObject transform = {};
				glm::mat4 depthBiasMVP = glm::mat4(1.f);
				glm::vec3 camPos;
				uLightObject light = {};
			} uniformDataScene{};

			struct UniformDataOffscreen 
			{
				glm::mat4 depthVP = glm::mat4(0.f);
			} uniformDataOffscreen{};

			struct {
				vk::Buffer scene = {};
				vk::Buffer offscreen = {};
			} uniformBuffers{};

			struct {
				uint32_t width, height = 0;
				VkFramebuffer frameBuffer = VK_NULL_HANDLE;
				FramebufferAttachment depth;
				VkRenderPass renderPass = VK_NULL_HANDLE;
				VkSampler depthSampler = VK_NULL_HANDLE;
			} offscreenPass{};


			//pipeline for scene here...
			VkPipeline offscreenPipeline = VK_NULL_HANDLE;
			VkPipeline offscreenDebugPipeline = VK_NULL_HANDLE;
			
			VkDescriptorSetLayout sceneDescriptorLayout = VK_NULL_HANDLE;
			
			struct {
				VkDescriptorSet offscreen = VK_NULL_HANDLE;
				VkDescriptorSet offscreenDebug = VK_NULL_HANDLE;
				VkDescriptorSet scene = VK_NULL_HANDLE;
			} descriptorSets{};

			const uint32_t shadowMapSize = 2048;

		public:
			ShadowMapScene();
			~ShadowMapScene();

			virtual void RecordCommandBuffers() override;
			virtual void InitializeScene(ObjectManager& objManager) override;
			virtual void ResizeWindow() override;

			virtual void Render();

		protected:
			virtual void InitializePipeline(std::string vsFile = "", std::string fsFile = "") override;
			virtual void InitializeDescriptors() override;
			virtual void FillOutGraphicsContextInfo() override;

			//class-specific methods.
		private:
			void InitializeOffscreenFramebuffer();
			void InitializeOffscreenRenderPass();
			void UpdateSceneUniforms();
			void UpdateOffscreenUniforms();
	};


}