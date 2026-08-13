#include "vkDeferredRenderer.h"
#include "vkInit.h"
#include "Camera.h"

namespace vk
{

	DeferredRenderer::DeferredRenderer( vk::Device& device, vk::Window& window, vk::TextureManager& textureManager,
		vk::DescriptorManager& descriptorManager ) :
		RendererBase(device, window, textureManager),
		c_descriptorManager(descriptorManager)
	{
		InitializeUniforms();
		InitializeFramebuffers();

		std::string skyboxName = "art/extern-textures/spruit_sunrise_2k.hdr";
		vk::TextureCreateInfo texture_create_info = {};
		texture_create_info.fileName = {skyboxName};
		texture_create_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		texture_create_info.mipLevels = 1;
		texture_create_info.layerCount = 1;

		m_test_panoramicImage = vk::PanoramicTexture(&c_device, texture_create_info);

		DeferredRenderer::InitializeDescriptors(c_descriptorManager);

		DeferredRenderer::InitializePipeline();
	}

	DeferredRenderer::~DeferredRenderer()
	{
		vkDestroyPipelineLayout(c_device.GetDevice(), m_graphicsPipelineLayout, nullptr);
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
			uniformBuffers.mrt[i] = c_device.CreateBuffer(sizeof(uniformDataMRT),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataMRT));
			uniformBuffers.mrt[i].Map(); //persistent data

			//////////////////////////////////
			//#2 - deferred shadows
			uniformBuffers.shadow[i] = c_device.CreateBuffer(sizeof(uniformDataDeferredShadow),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataDeferredShadow));
			uniformBuffers.shadow[i].Map(); //persistent data

			//////////////////////////////////
			//#3 - deferred light pass
			uniformBuffers.composition[i] = c_device.CreateBuffer(sizeof(uniformDataLightPass),
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
			c_descriptorManager.GetLayout(DescriptorCategory::eUBO),
			c_descriptorManager.GetLayout(DescriptorCategory::eCompositionImage),
			c_descriptorManager.GetLayout(DescriptorCategory::eMaterial)
		};


		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(pipelineLayout.size());
		pipelineLayoutCreateInfo.pSetLayouts = pipelineLayout.data();
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());

		VK_CHECK_RESULT(vkCreatePipelineLayout(c_device.GetDevice(), &pipelineLayoutCreateInfo,
					nullptr, &m_graphicsPipelineLayout));
	}

	void DeferredRenderer::UpdateScreenUniforms( const SceneView& sceneView )
	{
		Camera& sceneCam = *sceneView.camera;
		VkViewport windowViewport = c_window.Viewport();

		//transform(s)
		uniformDataMRT.viewMatrix = sceneCam.LookAt();

		uniformDataMRT.projectionMatrix = glm::perspective(glm::radians(sceneCam.GetFOV()),
				(float)windowViewport.width / windowViewport.height, 0.1f, 1000.f);
		uniformDataMRT.projectionMatrix[1][1] *= -1;

		memcpy(uniformBuffers.mrt[currentFrame].GetMappedMemory(), &uniformDataMRT,
			sizeof(uniformDataMRT));

		//shadows
		glm::mat4 perspective = glm::perspective(glm::radians(lightFOV), 1.f, zNear, zFar);

		uniformDataDeferredShadow.viewMatrices[0] = perspective * glm::lookAt(uniformDataLightPass.lights[0].pos,
			sceneSettings.freddyPosition, glm::vec3(0, 1, 0));

		uniformDataDeferredShadow.viewMatrices[1] = perspective * glm::lookAt(uniformDataLightPass.lights[1].pos,
			sceneSettings.cubePosition, glm::vec3(0, 1, 0));

		memcpy(uniformBuffers.shadow[currentFrame].GetMappedMemory(), &uniformDataDeferredShadow,
			sizeof(uniformDataDeferredShadow));
	}

	void DeferredRenderer::UpdateLights( const SceneView& sceneView )
	{
		//light(s)
		uniformDataLightPass.eyePosition = sceneView.camera->Position();
		uniformDataLightPass.lights[0].viewMatrix = uniformDataDeferredShadow.viewMatrices[0];
		uniformDataLightPass.lights[1].viewMatrix = uniformDataDeferredShadow.viewMatrices[1];

		memcpy(uniformBuffers.composition[currentFrame].GetMappedMemory(),
			&uniformDataLightPass,sizeof(uniformDataLightPass));
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

		if (UserInterface::CollapsingHeader("Deferred Context Settings"))
		{
			UserInterface::CheckBox("box test", &option);
			UserInterface::SeparatorText("light position");

			auto& lights = uniformDataLightPass.lights;
			for (size_t i = 0; i < lights.size(); ++i) 
			{
				UserInterface::Slider("light " + std::to_string(i), lights[i].pos);
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

					m_test_panoramicImage = PanoramicTexture(&c_device, texture_create_info);

					//have to update the descriptors
					InitializeSkyboxDescriptor( c_descriptorManager );

					InitializeEnvironmentMapDescriptors( c_descriptorManager );
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

		InitializeCompositionImageDescriptors(c_descriptorManager );
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