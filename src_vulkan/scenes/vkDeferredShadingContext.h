#pragma once
#include "vkContextBase.h"
#include "vkCubemap.h"

namespace vk 
{
	#define LIGHT_COUNT 2
	#define OBJECT_COUNT (10 + 1) //max 10 objects in the scene, +1 for blank texture

	//This scene is statically 4.2 KB!!!
	class DeferredContext : public ContextBase 
	{
	public:
		DeferredContext() = default;
		~DeferredContext() override;
		void Init() override;
		void Render( Scene& scene ) override;
		void ResizeWindow() override;
	protected:
		void RecordCommandBuffers( Scene& scene ) override;
		void UpdateUI() override;
		void InitializePipeline() override;
		void InitializeDescriptors() override;
	private:
		void InitializeFramebuffers();
		void InitializeDeferredFramebuffer();
		void InitializeDeferredShadowFramebuffer();
		void InitializeDeferredCompositionFramebuffer();
		void InitializeDeferredSkyboxFramebuffer();

		void InitializeUniforms();

		/*void InitializeCompositionSamplerDescriptor();
		void InitializeTextureSamplerDescriptor();
		void InitializeCompositionUniformDescriptor();
		void InitializeSwapChainDescriptor();
		void InitializeMRTDescriptor();
		void InitializeShadowMapDescriptor();
		void InitializeSkyBoxDescriptor();*/

		void InitializeGraphicsPipelineLayout();
		void UpdateScreenUniforms();
		void UpdateLights();
	private:
		enum dePipeline
		{
			COMPOSITION = 0,
			MRT,
			SHADOW,
			SKY,
			SWAPCHAIN, //non-uniformed pipeline
			PIPELINE_COUNT
		};

		enum DeferredRenderTargets
		{
			RT_POSITION = 0,
			RT_NORMAL,
			RT_ALBEDO,
			RT_METALLIC_ROUGHNESS,
			RT_AMBIENT_OCCLUSION,
			RT_COUNT
		};

		enum PBRMaterials
		{
			PBR_ALBEDO = 0,
			PBR_METALLIC_ROUGHNESS,
			PBR_AMBIENT_OCCLUSION,
			PBR_COUNT
		};

		struct Light
		{
			glm::vec3 pos        = glm::vec3(0.f); /* position of light */
			glm::vec3 albedo     = glm::vec3(1000.f); /* base color of light */
			glm::mat4 viewMatrix = glm::mat4(1.f); /* the viewpoint of the light toward a certain point */
		};

		struct UniformDataMRT
		{
			glm::mat4 eyeMatrix = glm::mat4(1.f);
			glm::mat4 projectionMatrix = glm::mat4(1.f);
		} uniformDataMRT{};

		struct UniformDataLightPass {
			std::array<Light, LIGHT_COUNT> lights;
			glm::vec3 eyePosition;
		} uniformDataLightPass {};

		struct UniformDataDeferredShadow
		{
			std::array<glm::mat4, LIGHT_COUNT> viewMatrices;
		} uniformDataDeferredShadow{};

		struct UniformBuffers
		{
			vk::Buffer mrt;
			vk::Buffer shadow;
			vk::Buffer composition;
		};

		std::array<UniformBuffers, gMaxFramesInFlight> uniformBuffers;
		vk::Buffer m_bigUniformBuffer;

		/*std::array<DescriptorBuffer, dePipeline::PIPELINE_COUNT> uniformBindingDescriptors;

		DescriptorBuffer skyboxSamplerDescriptor;
		DescriptorBuffer compositionSamplerDescriptor;
		DescriptorBuffer swapChainSamplerDescriptor;*/

		//NOTE: this will all be done offscreen because we have a main renderpass from the swapchain we'll
		//read the results of this from
		struct
		{
			std::array<Framebuffer, gMaxFramesInFlight> deMRT;
			std::array<Framebuffer, gMaxFramesInFlight> deShadow;
			std::array<Framebuffer, gMaxFramesInFlight> deComposition;
			std::array<Framebuffer, gMaxFramesInFlight> deSky;
		} framebuffers;

		//std::array<VkPipelineLayout, PIPELINE_COUNT> pipelineLayouts = {};
		VkPipelineLayout m_graphicsPipelineLayout;

		float depthBiasConstant = 1.25f;
		float depthBiasSlope    = 1.75f;

		float zNear    = 0.1f;
		float zFar     = 64.f;
		float lightFOV = 100.f;

		Cubemap test_cube;
	};

}