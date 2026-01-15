#pragma once
#include "vkContextBase.h"

namespace vk 
{
	#define LIGHT_COUNT 2

	//This scene is statically 5 MB!!!
	class DeferredContext : public ContextBase 
	{
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
			RT_COUNT
		};

		struct Light
		{
			float shininess = 0.f; /* exponent value */
			float _pad0[3];
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
			vk::Buffer mrt;
			vk::Buffer shadow;
			vk::Buffer composition;
		};

		std::array<UniformBuffers, gMaxFramesInFlight> uniformBuffers;

		struct DescriptorBufferData //240 BYTES!!!
		{
			std::array<vk::Buffer, gMaxFramesInFlight> buffers; //descriptors are stored in BUFFERS, not VkDescriptorSet
			std::vector<VkDeviceSize> binding_offsets = {0ull}; //at least 1 binding (binding 0)
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;
			VkDeviceSize size = 0ull;
			void Destroy() 
			{
				for (auto& b : buffers) 
				{
					b.Destroy();
				}
				vkDestroyDescriptorSetLayout(buffers[0].logicalDevice, layout, nullptr);
			}
		};

		std::array<DescriptorBufferData, dePipeline::PIPELINE_COUNT> uniformBindingDescriptors;
		//might be an abuse of map? Big memory cost.
		DescriptorBufferData textureBindingDescriptor;
		DescriptorBufferData compositionImageBindingDescriptor; //multiple frames in flight.

		//NOTE: this will all be done offscreen because we have a main renderpass from the swapchain we'll 
		//read the results of this from
		struct {
			Framebuffer deMRT;
			Framebuffer deShadow;
		} framebuffers;

		std::unique_ptr<Texture> defaultTexture;

		std::array<VkPipelineLayout, PIPELINE_COUNT> pipelineLayouts;

		float depthBiasConstant = 1.25f;
		float depthBiasSlope = 1.75f;

		float zNear = 0.1f;
		float zFar = 64.f;
		float lightFOV = 100.f;

		struct {
			glm::vec3 cubePosition = { 1.0, 20, -5.f };
			glm::vec3 freddyPosition = { 1.0f, 1.0, 5.f };
		} sceneSettings{};

		vkGltf::Model testGltfModel;

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
		void InitializeDescriptorBuffers();
		void InitializeDescriptorLayouts();
		void UpdateScreenUniforms();
		void UpdateLights();
		void DrawObjectsWithTexture(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, ObjectManager& objManager);

	};

}