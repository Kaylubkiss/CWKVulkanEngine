#include "vkDeferredRenderer.h"
#include "vkInit.h"
//TODO: be able to specify the objects you want in the scene at compile time.
//TODO: remove camel case -- looks ugly and a bit unreadable. There is some inconsistency here in this file with that. 


namespace vk
{

	DeferredRenderer::DeferredRenderer( TextureManager* textureManagerPtr,
		DescriptorManager& descriptorManagerPtr ) :
		m_descriptorManagerPtr(descriptorManagerPtr)
	{
		m_textureManagerPtr = textureManagerPtr;

		m_descriptorManagerPtr.Init(&device);
		m_textureManagerPtr->Init(&device, &m_descriptorManagerPtr);

		InitializeUniforms();
		InitializeFramebuffers();

		std::string skyboxName = "art/extern-textures/spruit_sunrise_2k.hdr";
		vk::TextureCreateInfo texture_create_info = {};
		texture_create_info.fileName = {skyboxName};
		texture_create_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		texture_create_info.mipLevels = 1;
		texture_create_info.layerCount = 1;

		m_test_panoramicImage = vk::PanoramicTexture(&device, texture_create_info);

		DeferredRenderer::InitializeDescriptors(m_descriptorManagerPtr);

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

		uniformDataLightPass.eyePosition = {0,0,0};
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

		for (size_t i = 0; i < gMaxFramesInFlight; ++i)
		{
			//////////////////////////////////
			//#1 - deferred MRT
			uniformBuffers.mrt[i] = device.CreateBuffer(sizeof(uniformDataMRT),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataMRT));
			uniformBuffers.mrt[i].Map(); //persistent data

			//////////////////////////////////
			//#2 - deferred shadows
			uniformBuffers.shadow[i] = device.CreateBuffer(sizeof(uniformDataDeferredShadow),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataDeferredShadow));
			uniformBuffers.shadow[i].Map(); //persistent data

			//////////////////////////////////
			//#3 - deferred light pass
			uniformBuffers.composition[i] = device.CreateBuffer(sizeof(uniformDataLightPass),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataLightPass));
			uniformBuffers.composition[i].Map(); //persistent data
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
			m_descriptorManagerPtr.GetLayout(DescriptorCategory::eUBO),
			m_descriptorManagerPtr.GetLayout(DescriptorCategory::eCompositionImage),
			m_descriptorManagerPtr.GetLayout(DescriptorCategory::eMaterial)
		};


		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(pipelineLayout.size());
		pipelineLayoutCreateInfo.pSetLayouts = pipelineLayout.data();
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());

		VK_CHECK_RESULT(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutCreateInfo,
					nullptr, &m_graphicsPipelineLayout));
	}

	void DeferredRenderer::UpdateScreenUniforms( const SceneView& sceneView )
	{
		VkViewport windowViewport = m_window.Viewport();

		//transform(s)

		Camera& sceneCam = *sceneView.camera;

		uniformDataMRT.viewMatrix = sceneCam.LookAt();

		uniformDataMRT.projectionMatrix = glm::perspective(glm::radians(sceneCam.GetFOV()),
				(float)windowViewport.width / windowViewport.height, 0.1f, 1000.f);

		uniformDataMRT.projectionMatrix[1][1] *= -1;

		memcpy(uniformBuffers.mrt[currentFrame].GetMappedMemory(), (void*)(&uniformDataMRT),
			sizeof(uniformDataMRT));

		//shadows
		glm::mat4 perspective = glm::perspective(glm::radians(lightFOV), 1.f, zNear, zFar);

		uniformDataDeferredShadow.viewMatrices[0] = perspective * glm::lookAt(uniformDataLightPass.lights[0].pos,
			sceneSettings.freddyPosition, glm::vec3(0, 1, 0));

		uniformDataDeferredShadow.viewMatrices[1] = perspective * glm::lookAt(uniformDataLightPass.lights[1].pos,
			sceneSettings.cubePosition, glm::vec3(0, 1, 0));

		memcpy(uniformBuffers.shadow[currentFrame].GetMappedMemory(), (void*)(&uniformDataDeferredShadow),
			sizeof(uniformDataDeferredShadow));
	}

	void DeferredRenderer::UpdateLights( const SceneView& sceneView )
	{
		//light(s)
		uniformDataLightPass.eyePosition = sceneView.camera->Position();
		uniformDataLightPass.lights[0].viewMatrix = uniformDataDeferredShadow.viewMatrices[0];
		uniformDataLightPass.lights[1].viewMatrix = uniformDataDeferredShadow.viewMatrices[1];

		memcpy(uniformBuffers.composition[currentFrame].GetMappedMemory(),
			(void*)(&uniformDataLightPass),
			sizeof(uniformDataLightPass));
	}

	std::vector<std::string> GetFileNamesOfExtension(const std::filesystem::path& directory, const char* extension)
	{
		std::vector<std::string> fileNames;

		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			if (entry.is_regular_file() == false)
			{
				continue;
			}

			std::string ext = entry.path().extension().string();
			std::ranges::transform(ext, ext.begin(), ::tolower);

			if (strcmp(ext.c_str(), extension) == 0)
			{
				fileNames.push_back(entry.path().filename().string());
			}
		}


		return fileNames;
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

			std::vector<std::string> fileNames = GetFileNamesOfExtension(
				"art/extern-textures/", ".hdr");

			std::vector<const char*> hdr_items;
			hdr_items.reserve(fileNames.size());
			for ( auto& file : fileNames )
			{
				hdr_items.push_back(file.c_str());
			}

			if (ImGui::Combo("HDR Environments", &m_guiHelper.selectedEnvironmentMap, hdr_items.data(),
				static_cast<int>(hdr_items.size())))
			{
				std::string skyboxName = "art/extern-textures/" + std::string(hdr_items[m_guiHelper.selectedEnvironmentMap]);

				if ( m_test_panoramicImage.GetName() != skyboxName )
				{

					vk::TextureCreateInfo texture_create_info = {  };
					texture_create_info.fileName = { skyboxName };
					texture_create_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
					texture_create_info.mipLevels = 1;
					texture_create_info.layerCount = 1;

					WaitForDevice();

					m_test_panoramicImage = PanoramicTexture(&device, texture_create_info);

					//have to update the descriptors
					InitializeSkyboxDescriptor( m_descriptorManagerPtr );

					InitializeEnvironmentMapDescriptors( m_descriptorManagerPtr );
				}
			}

		}
	}

	void DeferredRenderer::ResizeWindow()
	{
		RendererBase::ResizeWindow();

		InitializeFramebuffers();

		auto& mrtPipelineBuilder =
			pipelineManager.GetPipelineManager(dePipeline::MRT);

		if ( mrtPipelineBuilder != nullptr )
		{
			mrtPipelineBuilder->UpdateRenderPass(framebuffers.deMRT.back().GetRenderPass());
		}

		auto& compositionPipelineBuilder =
			pipelineManager.GetPipelineManager(dePipeline::COMPOSITION);

		if ( compositionPipelineBuilder != nullptr )
		{
			compositionPipelineBuilder->UpdateRenderPass(framebuffers.deComposition.back().GetRenderPass());
		}

		auto& skyboxPipelineBuilder =
			pipelineManager.GetPipelineManager(dePipeline::SKY);

		if ( skyboxPipelineBuilder != nullptr )
		{
			skyboxPipelineBuilder->UpdateRenderPass(framebuffers.deSky.back().GetRenderPass());
		}

		InitializeCompositionImageDescriptors(m_descriptorManagerPtr);
	}

	void DeferredRenderer::Render( SceneView sceneView  )
	{
		if (PrepareFrame())
		{ 
			UpdateScreenUniforms(sceneView);
			UpdateLights(sceneView);
			RecordCommandBuffers(sceneView);
			SubmitFrame();
		}
	}

}