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
		DeferredRenderer( vk::Device& device, vk::Window& window, vk::TextureManager& textureManager,
			vk::DescriptorManager& descriptorManager );
		~DeferredRenderer() override;

		void Render( SceneView sceneView  ) override;
	protected:
		void ResizeWindow() override;
		void UpdateUI() override;
	private:
		void RecordCommandBuffers( const SceneView& sceneView );
		void InitializePipeline();
		void InitializeDescriptors( vk::DescriptorManager& descriptorManager  );
		void InitializeFramebuffers();
		void InitializeDeferredFramebuffer();
		void InitializeDeferredShadowFramebuffer();
		void InitializeDeferredCompositionFramebuffer();

		void InitializeDeferredSkyboxFramebuffer();

		void InitializeUniforms();

		void InitializeUBODescriptors( DescriptorManager& descriptorManager );
		void InitializeCompositionImageDescriptors( DescriptorManager& descriptorManager );
		void InitializeSkyboxDescriptor( DescriptorManager& descriptorManager );
		void InitializeMaterialDescriptors( DescriptorManager& descriptorManager );
		void InitializeEnvironmentMapDescriptors( DescriptorManager& descriptorManager );

		void InitializePipelineLayouts();
		void UpdateScreenUniforms( const SceneView& sceneView );
		void UpdateLights( const SceneView& sceneView );
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

		struct UniformDataMRT
		{
			glm::mat4 viewMatrix = glm::mat4(1.f);
			glm::mat4 projectionMatrix = glm::mat4(1.f);
		} uniformDataMRT{};

		struct UniformDataLightPass
		{
			std::array<Light, LIGHT_COUNT> lights;
			glm::vec3 eyePosition = glm::vec3(0);
		} uniformDataLightPass {};

		struct UniformDataDeferredShadow
		{
			std::array<glm::mat4, LIGHT_COUNT> viewMatrices;
		} uniformDataDeferredShadow {};

		struct UniformBuffers
		{
			std::array<vk::Buffer, gMaxFramesInFlight> mrt;
			std::array<vk::Buffer, gMaxFramesInFlight> shadow;
			std::array<vk::Buffer, gMaxFramesInFlight> composition;
		} uniformBuffers;

		//for descriptorManager
		uint32_t mrtUBOLayoutIndex = 0;
		uint32_t shadowUBOLayoutIndex = 0;
		uint32_t lightUBOLayoutIndex = 0;

		uint32_t compositionImageIndex = 0;
		uint32_t swapChainImageIndex = 0;
		uint32_t skyboxImageIndex = UINT_MAX;

		struct
		{
			std::array<Framebuffer, gMaxFramesInFlight> deMRT;
			std::array<Framebuffer, gMaxFramesInFlight> deShadow;
			std::array<Framebuffer, gMaxFramesInFlight> deComposition;
			std::array<Framebuffer, gMaxFramesInFlight> deSky;
		} framebuffers;

		VkPipelineLayout m_graphicsPipelineLayout = VK_NULL_HANDLE;

		float depthBiasConstant = 1.25f;
		float depthBiasSlope    = 1.75f;

		float zNear    = 0.1f;
		float zFar     = 64.f;
		float lightFOV = 100.f;

		struct
		{
			glm::vec3 cubePosition   = { 1.0, 20, -5.f };
			glm::vec3 freddyPosition = { 1.5f, 1.0, 3.f };
		} sceneSettings {};

		struct
		{
			int selectedEnvironmentMap = 0;
		} m_guiHelper;

		vk::PanoramicTexture m_test_panoramicImage;

		vk::DescriptorManager& c_descriptorManager;

	};

}

#endif