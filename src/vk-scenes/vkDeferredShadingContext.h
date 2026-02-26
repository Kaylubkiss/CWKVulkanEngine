#pragma once
#include "vkContextBase.h"

namespace vk 
{
	#define LIGHT_COUNT 2

	//This scene is statically 4.2 MB!!!
	class DeferredContext : public ContextBase 
	{
	public:
		DeferredContext();
		~DeferredContext() override;

		void RecordCommandBuffers() override;
		void UpdateUI() override;
		void InitializeScene() override;
		void ResizeWindowDerived() override;
		void Render() override;
	protected:
		void InitializePipeline() override;
		void InitializeDescriptors() override;
		void FillOutGraphicsContextInfo() override;
	private:
		void InitializeDeferredFramebuffer();
		void InitializeDeferredShadowFramebuffer();
		void InitializeUniforms();
		void InitializeDescriptorBuffers();
		void InitializeDescriptorLayouts();
		void UpdateScreenUniforms();
		void UpdateLights();
	private:
		enum dePipeline
		{
			COMPOSITION = 0,
			MRT,
			SHADOW,
			PIPELINE_COUNT
		};

		enum DeferredRenderTargets
		{
			RT_POSITION = 0,
			RT_NORMAL,
			RT_ALBEDO,
			RT_METALLIC_ROUGHNESS,
			RT_COUNT
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
			//yikes, gonna have to copy each of these 16 floats from lights...
			std::array<glm::mat4, LIGHT_COUNT> viewMatrices;
		} uniformDataDeferredShadow{};

		struct UniformBuffers
		{
			vk::Buffer mrt;
			vk::Buffer shadow;
			vk::Buffer composition;
		};

		std::array<UniformBuffers, gMaxFramesInFlight> uniformBuffers;

		std::array<DescriptorBufferData, dePipeline::PIPELINE_COUNT> uniformBindingDescriptors;

		DescriptorBufferData textureBindingDescriptor;
		DescriptorBufferData compositionImageBindingDescriptor;

		//NOTE: this will all be done offscreen because we have a main renderpass from the swapchain we'll
		//read the results of this from
		struct {
			Framebuffer deMRT;
			Framebuffer deShadow;
		} framebuffers;

		std::array<VkPipelineLayout, PIPELINE_COUNT> pipelineLayouts = {};

		float depthBiasConstant = 1.25f;
		float depthBiasSlope    = 1.75f;

		float zNear    = 0.1f;
		float zFar     = 64.f;
		float lightFOV = 100.f;

		struct {
			glm::vec3 cubePosition   = { 1.0, 20, -5.f };
			glm::vec3 freddyPosition = { 1.0f, 1.0, 3.f };
		} sceneSettings{};

	};

}