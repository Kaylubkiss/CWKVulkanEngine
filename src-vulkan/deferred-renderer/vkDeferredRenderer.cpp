#include "vkDeferredRenderer.h"
#include "vkInit.h"
//TODO: be able to specify the objects you want in the scene at compile time.
//TODO: remove camel case -- looks ugly and a bit unreadable. There is some inconsistency here in this file with that. 


namespace vk
{

	DeferredRenderer::DeferredRenderer( TextureManager* textureManagerPtr,
		DescriptorManager* descriptorManagerPtr )
	{

		m_descriptorManagerPtr = descriptorManagerPtr;
		m_textureManagerPtr = textureManagerPtr;

		m_descriptorManagerPtr->Init(&device);
		m_textureManagerPtr->Init(&device, m_descriptorManagerPtr);

		InitializeUniforms();
		InitializeFramebuffers();

		std::string skyboxName = "art/extern-textures/spruit_sunrise_2k.hdr";
		vk::TextureCreateInfo texture_create_info = {  };
		texture_create_info.fileName = { skyboxName };
		texture_create_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		texture_create_info.mipLevels = 1;
		texture_create_info.layerCount = 1;

		m_test_panoramicImage = vk::PanoramicTexture(&device, texture_create_info);

		DeferredRenderer::InitializeDescriptors(*m_descriptorManagerPtr);

		DeferredRenderer::InitializePipeline();
	}

	DeferredRenderer::~DeferredRenderer()
	{
		vkDestroyPipelineLayout(device.GetDevice(), m_graphicsPipelineLayout, nullptr);
	}

	void DeferredRenderer::InitializeUniforms()
	{

		//initializing light positions
		uniformDataLightPass.lights[0].pos = { -7, 12, 3 };
		uniformDataLightPass.lights[1].pos = { -1, -7, 14 };

		uniformDataLightPass.eyePosition = mCamera.Position();
		uniformDataLightPass.lights[0].viewMatrix = uniformDataDeferredShadow.viewMatrices[0];
		uniformDataLightPass.lights[1].viewMatrix = uniformDataDeferredShadow.viewMatrices[1];

		//...position - light 0
		uniformDataLightPass.lights[0].albedo = glm::vec3(1000.f);

		//...position - light 1
		uniformDataLightPass.lights[1].albedo = uniformDataLightPass.lights[0].albedo;


		//shadow map view matrix
		glm::mat4 perspective = glm::perspective(glm::radians(lightFOV), 1.f, zNear, zFar);
		uniformDataDeferredShadow.viewMatrices[0] = perspective * glm::lookAt(uniformDataLightPass.lights[0].pos,
			sceneSettings.freddyPosition, glm::vec3(0, 1, 0));
		uniformDataDeferredShadow.viewMatrices[1] = perspective * glm::lookAt(uniformDataLightPass.lights[1].pos,
			sceneSettings.cubePosition, glm::vec3(0, 1, 0));

		for (size_t i = 0; i < uniformBuffers.size(); ++i)
		{
			//////////////////////////////////
			//#1 - deferred MRT
			uniformBuffers[i].mrt = device.CreateBuffer(sizeof(uniformDataMRT),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataMRT));
			uniformBuffers[i].mrt.Map(); //persistent data

			//////////////////////////////////
			//#2 - deferred shadows
			uniformBuffers[i].shadow = device.CreateBuffer(sizeof(uniformDataDeferredShadow),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataDeferredShadow));
			uniformBuffers[i].shadow.Map(); //persistent data

			//////////////////////////////////
			//#3 - deferred light pass
			uniformBuffers[i].composition = device.CreateBuffer(sizeof(uniformDataLightPass),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataLightPass));
			uniformBuffers[i].composition.Map(); //persistent data
		}
	}

	void DeferredRenderer::InitializePipelineLayouts()
	{

		std::vector<VkPushConstantRange> pushConstantRanges =
		{
			vk::init::PushConstantRange(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
		};

		std::vector<VkDescriptorSetLayout> pipelineLayout =
		{
			m_descriptorManagerPtr->GetLayout(DescriptorCategory::eUBO),
			m_descriptorManagerPtr->GetLayout(DescriptorCategory::eCompositionImage),
			m_descriptorManagerPtr->GetLayout(DescriptorCategory::eMaterial)
		};


		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(pipelineLayout.size());
		pipelineLayoutCreateInfo.pSetLayouts = pipelineLayout.data();
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());

		VK_CHECK_RESULT(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutCreateInfo,
					nullptr, &m_graphicsPipelineLayout));
	}

	void DeferredRenderer::UpdateScreenUniforms()
	{
		VkViewport windowViewport = m_window.Viewport();

		//transform(s)
		uniformDataMRT.eyeMatrix = mCamera.LookAt();

		uniformDataMRT.projectionMatrix = glm::perspective(glm::radians(cameraFOV),
				(float)windowViewport.width / windowViewport.height, 0.1f, 1000.f);

		uniformDataMRT.projectionMatrix[1][1] *= -1;

		memcpy(uniformBuffers[currentFrame].mrt.GetMappedMemory(), (void*)(&uniformDataMRT),
			sizeof(uniformDataMRT));

		//shadows
		glm::mat4 perspective = glm::perspective(glm::radians(lightFOV), 1.f, zNear, zFar);

		uniformDataDeferredShadow.viewMatrices[0] = perspective * glm::lookAt(uniformDataLightPass.lights[0].pos,
			sceneSettings.freddyPosition, glm::vec3(0, 1, 0));

		uniformDataDeferredShadow.viewMatrices[1] = perspective * glm::lookAt(uniformDataLightPass.lights[1].pos,
			sceneSettings.cubePosition, glm::vec3(0, 1, 0));

		memcpy(uniformBuffers[currentFrame].shadow.GetMappedMemory(), (void*)(&uniformDataDeferredShadow),
			sizeof(uniformDataDeferredShadow));
	}

	void DeferredRenderer::UpdateLights()
	{
		//light(s)
		uniformDataLightPass.eyePosition = mCamera.Position();
		uniformDataLightPass.lights[0].viewMatrix = uniformDataDeferredShadow.viewMatrices[0];
		uniformDataLightPass.lights[1].viewMatrix = uniformDataDeferredShadow.viewMatrices[1];

		memcpy(uniformBuffers[currentFrame].composition.GetMappedMemory(),
			(void*)(&uniformDataLightPass),
			sizeof(uniformDataLightPass));
	}

	void DeferredRenderer::UpdateUI()
	{
		static bool option = false;

		if (pipelineManager.HotReloadIsReady())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Shaders Out of Date");
			m_settings.hotReloadRequested = ImGui::Button("Hot Reload");
		}

		if (UIOverlay.CollapsingHeader("Deferred Context Settings"))
		{
			UIOverlay.CheckBox("box test", &option);
			UIOverlay.SeparatorText("light position");

			auto& lights = uniformDataLightPass.lights;
			for (size_t i = 0; i < lights.size(); ++i) 
			{
				UIOverlay.Slider("light " + std::to_string(i), lights[i].pos);
			}
			UIOverlay.SeparatorText("textures in scene");
			UIOverlay.DisplayImages();
		}
	}

	void DeferredRenderer::ResizeWindow()
	{
		RendererBase::ResizeWindow();

		InitializeFramebuffers();

		auto& mrtPipelineBuilder =
			pipelineManager.GetPipelineManager(dePipeline::MRT);
		if (mrtPipelineBuilder != nullptr)
		{
			mrtPipelineBuilder->UpdateRenderPass(framebuffers.deMRT.back().GetRenderPass());
		}

		auto& compositionPipelineBuilder =
			pipelineManager.GetPipelineManager(dePipeline::COMPOSITION);
		if (compositionPipelineBuilder != nullptr)
		{
			compositionPipelineBuilder->UpdateRenderPass(framebuffers.deComposition.back().GetRenderPass());
		}

		auto& skyboxPipelineBuilder = pipelineManager.GetPipelineManager(dePipeline::SKY);
		if (skyboxPipelineBuilder != nullptr)
		{
			skyboxPipelineBuilder->UpdateRenderPass(framebuffers.deSky.back().GetRenderPass());
		}

		InitializeCompositionImageDescriptors(*m_descriptorManagerPtr);
	}

	void DeferredRenderer::Render( AssetManager& assetManager )
	{
		if (PrepareFrame())
		{ 
			UpdateScreenUniforms();
			UpdateLights();
			RecordCommandBuffers(assetManager);
			SubmitFrame();
		}
	}

}