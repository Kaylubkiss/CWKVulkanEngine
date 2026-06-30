#ifndef VK_DEFERRED_RENDERER_HPP
#define VK_DEFERRED_RENDERER_HPP

#include "vkRendererBase.h"
#include "vkPanoramicTexture.h"

namespace vk 
{
	inline constexpr uint32_t LIGHT_COUNT = 2;
	inline constexpr uint32_t OBJECT_COUNT = (10000 + 1); //max 10 objects in the scene, +1 for blank texture

	class DeferredRenderer : public RendererBase
	{
	public:
		DeferredRenderer( TextureManager* textureManagerPtr, DescriptorManager* descriptorManagerPtr );
		~DeferredRenderer() override;

		void Render( AssetManager& assetManager ) override;
		void ResizeWindow() override;
	protected:
		void RecordCommandBuffers( AssetManager& assetManager ) override;
		void UpdateUI() override;
		void InitializePipeline() override;
		void InitializeDescriptors( vk::DescriptorManager& descriptorManager  ) override;
	private:
		void InitializeFramebuffers();
		void InitializeDeferredFramebuffer();
		void InitializeDeferredShadowFramebuffer();
		void InitializeDeferredCompositionFramebuffer();

		void InitializeDeferredSkyboxFramebuffer();

		void InitializeUniforms();

		void InitializeUBODescriptors( DescriptorManager& descriptorManager );
		void InitializeCompositionImageDescriptors( DescriptorManager& descriptorManager );
		void InitializeMaterialDescriptors( DescriptorManager& descriptorManager );

		void InitializePipelineLayouts();
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

		//for descriptorManager
		uint32_t mrtUBOLayoutIndex = 0;
		uint32_t shadowUBOLayoutIndex = 0;
		uint32_t lightUBOLayoutIndex = 0;

		uint32_t compositionImageIndex = 0;
		uint32_t swapChainImageIndex = 0;

		uint32_t skyboxImageIndex = 0;

		std::array<UniformBuffers, gMaxFramesInFlight> uniformBuffers;

		struct
		{
			std::array<Framebuffer, gMaxFramesInFlight> deMRT; //NOTE: should be an array
			std::array<Framebuffer, gMaxFramesInFlight> deShadow;
			std::array<Framebuffer, gMaxFramesInFlight> deComposition;
			std::array<Framebuffer, gMaxFramesInFlight> deSky;
		} framebuffers;

		VkPipelineLayout m_graphicsPipelineLayout = nullptr;

		float depthBiasConstant = 1.25f;
		float depthBiasSlope    = 1.75f;

		float zNear    = 0.1f;
		float zFar     = 64.f;
		float lightFOV = 100.f;

		struct
		{
			glm::vec3 cubePosition   = { 1.0, 20, -5.f };
			glm::vec3 freddyPosition = { 1.5f, 1.0, 3.f };
		} sceneSettings{};

		DescriptorManager* m_descriptorManagerPtr = nullptr;

		vk::PanoramicTexture m_test_panoramicImage;

	};

}

#endif