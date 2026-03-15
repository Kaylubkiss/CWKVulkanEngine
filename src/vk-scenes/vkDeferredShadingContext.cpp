#include "vkDeferredShadingContext.h"
//TODO: be able to specify the objects you want in the scene at compile time.
//TODO: remove camel case -- looks ugly and a bit unreadable. There is some inconsistency here in this file with that. 


namespace vk
{

	DeferredContext::DeferredContext()
	{
		std::mutex testMutex;
		test_cube.Create(&this->device, "pooop", testMutex);

		vk::Device* devicePtr = &this->device;

		VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkImageMemoryBarrier acquireBarrier = {};
		acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		acquireBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		acquireBarrier.srcQueueFamilyIndex = devicePtr->GetQueue(vk::DeviceQueue::TRANSFER).family;
		acquireBarrier.dstQueueFamilyIndex = devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family;
		acquireBarrier.image = test_cube.GetImage();
		acquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		acquireBarrier.subresourceRange.baseMipLevel = 0;
		acquireBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		acquireBarrier.subresourceRange.baseArrayLayer = 0;
		acquireBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		acquireBarrier.srcAccessMask = 0;
		acquireBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffers[currentFrame], &cmdBufferBeginInfo));

		vkCmdPipelineBarrier(
			commandBuffers[currentFrame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr,
			0, nullptr,
			1, &acquireBarrier
		);

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers[currentFrame]));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).handle, 1, &submitInfo, VK_NULL_HANDLE));

		VK_CHECK_RESULT(vkQueueWaitIdle(devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).handle));

		InitializeUniforms();
		InitializeFramebuffers();

		DeferredContext::InitializeDescriptors();

		std::array<VkDescriptorSetLayoutBinding, 3> setLayoutBindings = {};

		//albedo
		setLayoutBindings[0].binding = 0;
		setLayoutBindings[0].descriptorCount = 1;
		setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		//metallic roughness
		setLayoutBindings[1].binding = 1;
		setLayoutBindings[1].descriptorCount = 1;
		setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		//ambient occlusion
		setLayoutBindings[2].binding = 2;
		setLayoutBindings[2].descriptorCount = 1;
		setLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		DescriptorBufferCreateInfo descriptorBufferCI = {};
		descriptorBufferCI.devicePtr = &device;
		descriptorBufferCI.bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		descriptorBufferCI.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		descriptorBufferCI.pLayoutBindings = setLayoutBindings.data();
		descriptorBufferCI.layoutBindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorBufferCI.imageDescriptorData.resize(OBJECT_COUNT);

		m_info->descriptorBufferCreateInfoPtr = &descriptorBufferCI;

		//TODO: not happy that I have to initialize a protected member here...
		//but... I have to further test my implementations and fix the async issue.
		m_assetManager = std::make_unique<AssetManager>(m_info);

		DeferredContext::InitializePipeline();
		DeferredContext::FillOutGraphicsContextInfo();


	}

	DeferredContext::~DeferredContext()
	{
		for (size_t i = 0; i < uniformBuffers.size(); ++i)
		{
			uniformBuffers[i].mrt.Destroy();
			uniformBuffers[i].composition.Destroy();
			uniformBuffers[i].shadow.Destroy();
		}

		//because the layouts are initialized in one function, we can
		//assume the check here
		if (pipelineLayouts[0] != VK_NULL_HANDLE)
		{
			for (auto& pl : pipelineLayouts)
			{
				vkDestroyPipelineLayout(device.GetDevice(), pl, nullptr);
			}
		}

		for (auto& uniformDescriptor : uniformBindingDescriptors)
		{
			uniformDescriptor.Destroy();
		}

		compositionImageBindingDescriptor.Destroy();
		skyboxSamplerBindingDescriptor.Destroy();
		swapChainSamplerBindingDescriptor.Destroy();

		for (size_t frame = 0; frame < gMaxFramesInFlight; ++frame)
		{
			framebuffers.deMRT[frame].Destroy();
			framebuffers.deShadow[frame].Destroy();
			framebuffers.deSky[frame].Destroy();
			framebuffers.deComposition[frame].Destroy();
		}
	}

	void DeferredContext::InitializeScene()
	{
		ObjectCreateInfo objectCI = {};
		
		//object 1 - freddy
		objectCI.objName = "freddy.obj";
		objectCI.textureFileName = "art/extern-textures/myface.JPG";
		objectCI.modelTransform = glm::translate(glm::mat4(1.f), sceneSettings.freddyPosition) * 
			glm::scale(glm::mat4(1.f), glm::vec3(3.f));
		objectCI.devicePtr = &this->device;

		m_assetManager->LoadObject(objectCI);

		//object 2 - cube
		objectCI = {};

		PhysicsComponent physicsComponent;
		physicsComponent.bodyType = BodyType::DYNAMIC;
		physicsComponent.colliderType = PhysicsComponent::ColliderType::CUBE;
		
		objectCI.objName = "cube.obj";
		//NOTE: cube.obj doesn't have UVs.
		objectCI.textureFileName = "art/extern-textures/myface.JPG";
		objectCI.physicsComponent = physicsComponent;
		objectCI.hasPhysicsComponent = true;
		objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(sceneSettings.cubePosition));
		objectCI.devicePtr = &this->device;

		m_assetManager->LoadObject(objectCI);

		//object 3 - base
		objectCI = {};
		
		physicsComponent.bodyType = reactphysics3d::BodyType::STATIC;

		objectCI.objName = "base.obj";
		objectCI.textureFileName = "art/extern-textures/wood-floor.png";
		objectCI.physicsComponent = physicsComponent;
		objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0, -5.f, 0)) *
			glm::scale(glm::mat4(1.f), glm::vec3(30.f));
		objectCI.hasPhysicsComponent = true;
		objectCI.devicePtr = &this->device;

		m_assetManager->LoadObject(objectCI);

		objectCI = {};

		objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(-3, 1.f, 0));
		objectCI.objName = "AnimatedCube/glTF/AnimatedCube.gltf";
		objectCI.devicePtr = &this->device;

		m_assetManager->LoadObject(objectCI);

		objectCI = {};
		objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0));
		objectCI.objName = "SciFiHelmet/glTF/SciFiHelmet.gltf";
		objectCI.devicePtr = &this->device;

		m_assetManager->LoadObject(objectCI);

		//initializing light positions
		uniformDataLightPass.lights[0].pos = { 3, 27, -14 };
		uniformDataLightPass.lights[1].pos = { 33, 33, 30 };

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
	}

	void DeferredContext::InitializeUniforms()
	{
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

	void DeferredContext::InitializePipelineLayouts()
	{
		auto& textureManager = m_assetManager->GetTextureManager();

		//MRT PASS LAYOUT
		{
			if (pipelineLayouts[dePipeline::MRT] == VK_NULL_HANDLE)
			{
				auto& textureSamplerDescriptor = textureManager.GetTextureSamplerDescriptor();

				std::vector<VkPushConstantRange> pushConstantRanges =
				{
					vk::init::PushConstantRange(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
				};

				std::array<VkDescriptorSetLayout, 2> mrt_layouts =
				{
					//set 0: per-frame scene transform, set 1: per-model image sampler(s)
					uniformBindingDescriptors[dePipeline::MRT].GetLayout(), textureSamplerDescriptor.GetLayout(),
				};

				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
				pipelineLayoutCreateInfo.pSetLayouts = mrt_layouts.data();
				pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(mrt_layouts.size());
				pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
				pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

				VK_CHECK_RESULT(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutCreateInfo,
					nullptr, &pipelineLayouts[dePipeline::MRT]));
			}
		}

		//COMPOSITION PASS LAYOUT
		{
			if (pipelineLayouts[dePipeline::COMPOSITION] == VK_NULL_HANDLE)
			{
				//order of layouts need to be in order of they appear in shader(s)
				std::array<VkDescriptorSetLayout, 2> composition_layouts = {
					//set 0: image samplers, set 1: light ubo
					compositionImageBindingDescriptor.GetLayout(), uniformBindingDescriptors[dePipeline::COMPOSITION].GetLayout()
				};

				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
				pipelineLayoutCreateInfo.pSetLayouts = composition_layouts.data();
				pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(composition_layouts.size());

				VK_CHECK_RESULT(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutCreateInfo,
					nullptr, &pipelineLayouts[dePipeline::COMPOSITION]));
			}
		}


		//SHADOW MAP LAYOUT
		{
			if (pipelineLayouts[dePipeline::SHADOW] == VK_NULL_HANDLE)
			{
				//set 0: shadow UBO - per frame
				std::array<VkDescriptorSetLayout, 1> shadow_layouts = {
					uniformBindingDescriptors[dePipeline::SHADOW].GetLayout()
				};
				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
				pipelineLayoutCreateInfo.pSetLayouts = shadow_layouts.data();
				pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(shadow_layouts.size());
				//per-model transform
				std::vector<VkPushConstantRange> pushConstantRanges = {
					vk::init::PushConstantRange(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
				};
				pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
				pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());

				VK_CHECK_RESULT(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutCreateInfo,
					nullptr, &pipelineLayouts[dePipeline::SHADOW]));
			}
		}


		//SKYBOX LAYOUT
		{
			if (pipelineLayouts[dePipeline::SKY] == VK_NULL_HANDLE)
			{
				std::array<VkDescriptorSetLayout, 2> layouts =
				{
					//set 0: uniforms, set 1: samplers
					uniformBindingDescriptors[dePipeline::MRT].GetLayout(), skyboxSamplerBindingDescriptor.GetLayout()
				};

				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
				pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
				pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

				VK_CHECK_RESULT(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutCreateInfo,
					nullptr, &pipelineLayouts[dePipeline::SKY]));
			}
		}

		//SWAPCHAIN QUAD LAYOUT
		{
			if (pipelineLayouts[dePipeline::SWAPCHAIN] == VK_NULL_HANDLE)
			{
				std::array<VkDescriptorSetLayout, 1> layouts = {
					//set 0: scene sampler
					swapChainSamplerBindingDescriptor.GetLayout()
				};

				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
				pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
				pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

				VK_CHECK_RESULT(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutCreateInfo,
					nullptr, &pipelineLayouts[dePipeline::SWAPCHAIN]));
			}
		}
	}

	void DeferredContext::UpdateScreenUniforms()
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

	void DeferredContext::UpdateLights() 
	{
		//light(s)
		uniformDataLightPass.eyePosition = mCamera.Position();
		uniformDataLightPass.lights[0].viewMatrix = uniformDataDeferredShadow.viewMatrices[0];
		uniformDataLightPass.lights[1].viewMatrix = uniformDataDeferredShadow.viewMatrices[1];

		memcpy(uniformBuffers[currentFrame].composition.GetMappedMemory(), (void*)(&uniformDataLightPass),
			sizeof(uniformDataLightPass));
	}

	void DeferredContext::UpdateUI() 
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

	void DeferredContext::ResizeWindow()
	{
		ContextBase::ResizeWindow();

		InitializeFramebuffers();

		//need to recreate these as their image layouts turn stale
		compositionImageBindingDescriptor.Destroy();
		swapChainSamplerBindingDescriptor.Destroy();

		InitializeCompositionSamplerDescriptor();
		InitializeSwapChainDescriptor();
	}

	void DeferredContext::Render() 
	{
		if (PrepareFrame())
		{ 
			UpdateScreenUniforms();
			UpdateLights();
			RecordCommandBuffers();
			SubmitFrame();
		}
	}

}