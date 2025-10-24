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
			MRT,
			SHADOW
		};

		enum DeferredRenderTargets 
		{
			RT_POSITION = 0,
			RT_NORMAL,
			RT_ALBEDO
		};

		struct Light
		{
			float shininess = 0.f; /* exponent value */

			glm::vec3 pos = glm::vec3(0.f); /* position of light */
			glm::vec3 ambient = glm::vec3(0.f); /* scene color */
			glm::vec3 albedo = glm::vec3(0.f); /* base color of light */
			glm::vec3 specular = glm::vec3(0.f); /* reflectivity of the light */

			glm::mat4 viewMatrix;
		};

		struct UniformDataMRT
		{
			uTransformObject uTransform;
		} uniformDataMRT{};

		
		struct UniformDataLightPass {
			std::array<Light, LIGHT_COUNT> lights;
			glm::vec3 viewPosition;
		} uniformDataLightPass {};

		struct UniformDataDeferredShadow 
		{
			std::array<glm::mat4, LIGHT_COUNT> viewMatrices; //yikes, gonna have to copy each of these 16 floats from lights...
		} uniformDataDeferredShadow{};

		struct UniformBuffers
		{
			vk::Buffer deferredMRT;
			vk::Buffer deferredShadow;
			vk::Buffer composition;
		};

		std::array<UniformBuffers, gMaxFramesInFlight> uniformBuffers;

		VkDescriptorSetLayout sceneDescriptorSetLayout = VK_NULL_HANDLE;

		//NOTE: this will all be done offscreen because we have a main renderpass from the swapchain we'll 
		//read the results of this from
		Framebuffer deferredMRTFB;
		Framebuffer deferredShadowFB;

		struct DescriptorSets
		{
			VkDescriptorSet deferred;
			VkDescriptorSet deferredShadow;
			VkDescriptorSet composition;
		};
		
		std::array<DescriptorSets, gMaxFramesInFlight> descriptorSets;

		Texture defaultTexture;

		float depthBiasConstant = 1.25f;
		float depthBiasSlope = 1.75f;

		float zNear = 0.1f;
		float zFar = 64.f;
		float lightFOV = 100.f;

		struct {
			glm::vec3 cubePosition = { 0, 20, -5.f };
			glm::vec3 freddyPosition = { 1.0f, 0, 5.f };
		} sceneSettings{};

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
		void InitializeDeferredShadowFramebuffer();
		void InitializeUniforms();
		void UpdateScreenUniforms();
		void UpdateSceneUniforms();

	};

}