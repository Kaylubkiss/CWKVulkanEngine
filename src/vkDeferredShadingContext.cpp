#include "vkDeferredShadingContext.h"

namespace vk 
{

	DeferredContext::DeferredContext()
	{

		deferredMRTFB.width = 2048;
		deferredMRTFB.height = 2048;

		deferredShadowFB.width = 2048;
		deferredShadowFB.height = 2048;


		defaultTexture = Texture(&this->mInfo, "wood-floor.png");
		if (settings.UIEnabled) 
		{
			UIOverlay.AddImage(defaultTexture);
		}

		DeferredContext::InitializeUniforms();
		DeferredContext::IntializeDeferredFramebuffer();
		DeferredContext::InitializeDeferredShadowFramebuffer();
		DeferredContext::InitializeDescriptors();
		DeferredContext::InitializePipeline();

		FillOutGraphicsContextInfo();
		
	}

	DeferredContext::~DeferredContext() 
	{
		for (size_t i = 0; i < uniformBuffers.size(); ++i) 
		{
			uniformBuffers[i].deferredMRT.Destroy();

			uniformBuffers[i].composition.Destroy();

			uniformBuffers[i].deferredShadow.Destroy();
		}

		defaultTexture.Destroy(device.logical);

		deferredMRTFB.Destroy();
		
		deferredShadowFB.Destroy();

		vkDestroyDescriptorSetLayout(device.logical, this->sceneDescriptorSetLayout, nullptr);
	}

	void DeferredContext::InitializeScene(ObjectManager& objManager) 
	{
		glm::mat4 modelTransform = glm::mat4(5.f);
		modelTransform[3] = glm::vec4(sceneSettings.freddyPosition, 1);

		//object 1 - freddy
		ObjectCreateInfo objectCI;
		objectCI.objName = "freddy.obj";
		objectCI.textureFileName = "myface.JPG";
		objectCI.pModelTransform = &modelTransform;
		objectCI.devicePtr = &this->device;

		objManager.LoadObject(objectCI);

		//object 2 - cube
		modelTransform = glm::mat4(1.f);
		modelTransform[3] = glm::vec4(sceneSettings.cubePosition, 1);

		PhysicsComponent physicsComponent;
		physicsComponent.bodyType = BodyType::DYNAMIC;
		physicsComponent.colliderType = PhysicsComponent::ColliderType::CUBE;

		objectCI = {};
		objectCI.objName = "cube.obj";
		objectCI.textureFileName = "texture.jpg";
		objectCI.pPhysicsComponent = &physicsComponent;
		objectCI.pModelTransform = &modelTransform;
		objectCI.devicePtr = &this->device;

		objManager.LoadObject(objectCI);

		//object 3 - base
		const float dbScale = 30.f;
		modelTransform = glm::mat4(dbScale);
		modelTransform[3] = { 0.f, -5.f, 0.f, 1 };

		physicsComponent.bodyType = reactphysics3d::BodyType::STATIC;


		objectCI = {};
		objectCI.objName = "base.obj";
		objectCI.textureFileName = "";
		objectCI.pPhysicsComponent = &physicsComponent;
		objectCI.pModelTransform = &modelTransform;
		objectCI.devicePtr = &this->device;

		objManager.LoadObject(objectCI);

	}

	void DeferredContext::ResizeWindowDerived()
	{
		VkExtent2D windowExtents = window.Extents();

		//MRT resizing...
	/*	deferredMRTFB.width = windowExtents.width;
		deferredMRTFB.height = windowExtents.height;*/

		for (auto& attachment : deferredMRTFB.attachments)
		{
			attachment.Destroy(device.logical);
		}

		deferredMRTFB.attachments.resize(0);

		VkFramebufferCreateInfo framebuffer = vk::init::FramebufferCreateInfo();
		framebuffer.width = deferredMRTFB.width;
		framebuffer.height = deferredMRTFB.height;
		framebuffer.layers = 1;

		vk::FramebufferAttachmentCreateInfo attachmentCI = {};

		//position attachment
		attachmentCI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		attachmentCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		attachmentCI.width = framebuffer.width;
		attachmentCI.height = framebuffer.height;
		deferredMRTFB.AddAttachment(attachmentCI);

		//normal attachment
		deferredMRTFB.AddAttachment(attachmentCI);

		//albedo attachment
		attachmentCI.format = VK_FORMAT_R8G8B8A8_UNORM;
		deferredMRTFB.AddAttachment(attachmentCI);

		//depth attachment
		attachmentCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
		attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		deferredMRTFB.AddAttachment(attachmentCI);

		deferredMRTFB.CreateFramebuffer();

		//shadow resizing...

		for (auto& attachment : deferredShadowFB.attachments)
		{
			attachment.Destroy(device.logical);
		}

		deferredShadowFB.attachments.resize(0);

		attachmentCI = {};
		attachmentCI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		attachmentCI.width = deferredShadowFB.width;
		attachmentCI.height = deferredShadowFB.height;
		attachmentCI.layerCount = LIGHT_COUNT;
		attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		deferredShadowFB.AddAttachment(attachmentCI);

		deferredShadowFB.CreateFramebuffer();

		//writing descriptor sets
		std::vector<VkDescriptorImageInfo> descriptorImage; //position, normal, and albedo attachments

		for (const auto& attachment : deferredMRTFB.attachments)
		{
			if (attachment.flags & VKC_ATTACHMENT_IS_COLOR)
			{
				VkDescriptorImageInfo descInfo = {};
				descInfo.imageLayout = attachment.description.finalLayout;
				descInfo.imageView = attachment.imageView;
				descInfo.sampler = deferredMRTFB.sampler;

				descriptorImage.push_back(descInfo);
			}
		}

		VkDescriptorImageInfo shadowMapDescriptor;
		shadowMapDescriptor.imageLayout = deferredShadowFB.attachments[0].description.finalLayout;
		shadowMapDescriptor.imageView = deferredShadowFB.attachments[0].imageView;
		shadowMapDescriptor.sampler = deferredShadowFB.sampler;


		for (size_t i = 0; i < descriptorSets.size(); ++i) 
		{
			//composition
			std::vector<VkWriteDescriptorSet>  writeDescriptorSets = {
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].composition.descriptor),
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &descriptorImage[RT_POSITION]),
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &descriptorImage[RT_NORMAL]),
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &descriptorImage[RT_ALBEDO]),
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &shadowMapDescriptor)
			};
			vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

			//shadow write
			writeDescriptorSets = {
				vk::init::WriteDescriptorSet(descriptorSets[i].deferredShadow, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].deferredShadow.descriptor),
			};
			vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
		}


	}

	void DeferredContext::FillOutGraphicsContextInfo() 
	{
		mInfo.descriptorPool = this->descriptorPool;
		mInfo.descriptorSetLayout = this->sceneDescriptorSetLayout;
		mInfo.samplerBinding = 3;
	}

	void DeferredContext::InitializeUniforms()
	{
		for (size_t i = 0; i < uniformBuffers.size(); ++i) {
			//////////////////////////////////
			//#1 - deferred MRT
			uniformBuffers[i].deferredMRT = device.CreateBuffer(sizeof(uniformDataMRT), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)(&uniformDataMRT));
			uniformBuffers[i].deferredMRT.Map();


			//initializing light positions
			uniformDataLightPass.lights[0].pos = { 9, 9, 21 };
			uniformDataLightPass.lights[1].pos = { 52, 3, 9 };

			//////////////////////////////////
			//#2 - deferred shadow
			glm::mat4 perspective = glm::perspective(glm::radians(lightFOV), 1.f, zNear, zFar);
			uniformDataDeferredShadow.viewMatrices[0] = perspective * glm::lookAt(uniformDataLightPass.lights[0].pos, sceneSettings.freddyPosition, glm::vec3(0, 1, 0));
			uniformDataDeferredShadow.viewMatrices[1] = perspective * glm::lookAt(uniformDataLightPass.lights[1].pos, sceneSettings.cubePosition, glm::vec3(0, 1, 0));

			uniformBuffers[i].deferredShadow = device.CreateBuffer(sizeof(uniformDataDeferredShadow), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)(&uniformDataDeferredShadow));
			uniformBuffers[i].deferredShadow.Map();

			//////////////////////////////////
			//#3 - deferred light pass
			uniformDataLightPass.viewPosition = mCamera.Position();
			uniformDataLightPass.lights[0].viewMatrix = uniformDataDeferredShadow.viewMatrices[0];
			uniformDataLightPass.lights[1].viewMatrix = uniformDataDeferredShadow.viewMatrices[1];

			//...position - light 0
			uniformDataLightPass.lights[0].albedo = { 1.0, 1.0, 1.0 };
			uniformDataLightPass.lights[0].ambient = uniformDataLightPass.lights[0].albedo * 0.1f;
			uniformDataLightPass.lights[0].specular = { 0.5f, 0.5f, 0.5f };
			uniformDataLightPass.lights[0].shininess = 32.f;

			//...position - light 1
			uniformDataLightPass.lights[1].albedo = uniformDataLightPass.lights[0].albedo;
			uniformDataLightPass.lights[1].ambient = uniformDataLightPass.lights[0].ambient;
			uniformDataLightPass.lights[1].specular = uniformDataLightPass.lights[0].specular;
			uniformDataLightPass.lights[1].shininess = uniformDataLightPass.lights[0].shininess;

			uniformBuffers[i].composition = device.CreateBuffer(sizeof(uniformDataLightPass), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)(&uniformDataLightPass));
			uniformBuffers[i].composition.Map();
		}
	}

	void DeferredContext::UpdateScreenUniforms()
	{
		//transform(s)
		uniformDataMRT.uTransform =
		{
			mCamera.LookAt(),
			glm::perspective(glm::radians(FOV), (float)window.viewport.width / window.viewport.height, 0.1f, 1000.f)
		};

		uniformDataMRT.uTransform.proj[1][1] *= -1;

		memcpy(uniformBuffers[currentFrame].deferredMRT.mappedMemory, (void*)(&uniformDataMRT), sizeof(uniformDataMRT));
	}

	void DeferredContext::UpdateSceneUniforms() 
	{
		//shadows
		glm::mat4 perspective = glm::perspective(glm::radians(lightFOV), 1.f, zNear, zFar);
		uniformDataDeferredShadow.viewMatrices[0] = perspective * glm::lookAt(uniformDataLightPass.lights[0].pos, sceneSettings.freddyPosition, glm::vec3(0, 1, 0));
		uniformDataDeferredShadow.viewMatrices[1] = perspective * glm::lookAt(uniformDataLightPass.lights[1].pos, sceneSettings.cubePosition, glm::vec3(0, 1, 0));
		memcpy(uniformBuffers[currentFrame].deferredShadow.mappedMemory, (void*)(&uniformDataDeferredShadow), sizeof(uniformDataDeferredShadow));

		//light(s)
		uniformDataLightPass.viewPosition = mCamera.Position();
		uniformDataLightPass.lights[0].viewMatrix = uniformDataDeferredShadow.viewMatrices[0];
		uniformDataLightPass.lights[1].viewMatrix = uniformDataDeferredShadow.viewMatrices[1];
		memcpy(uniformBuffers[currentFrame].composition.mappedMemory, (void*)(&uniformDataLightPass), sizeof(uniformDataLightPass));

		
	}

	void DeferredContext::IntializeDeferredFramebuffer() 
	{
		VkExtent2D windowExtents = window.Extents();
		deferredMRTFB.Init(&this->device, windowExtents);

		VkFramebufferCreateInfo framebuffer = vk::init::FramebufferCreateInfo();
		framebuffer.width = deferredMRTFB.width;
		framebuffer.height = deferredMRTFB.height;
		framebuffer.layers = 1;
		
		vk::FramebufferAttachmentCreateInfo attachmentCI = {};

		//position attachment
		attachmentCI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		attachmentCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		attachmentCI.width = framebuffer.width;
		attachmentCI.height = framebuffer.height;
		deferredMRTFB.AddAttachment(attachmentCI);
		
		//normal attachment
		deferredMRTFB.AddAttachment(attachmentCI);

		//albedo attachment
		attachmentCI.format = VK_FORMAT_R8G8B8A8_UNORM;
		deferredMRTFB.AddAttachment(attachmentCI);

		//depth attachment
		attachmentCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
		attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		deferredMRTFB.AddAttachment(attachmentCI);

		deferredMRTFB.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		deferredMRTFB.CreateRenderPass();

		deferredMRTFB.CreateFramebuffer();

	}

	void DeferredContext::InitializeDeferredShadowFramebuffer() 
	{
		VkExtent2D windowExtents = window.Extents();
		deferredShadowFB.Init(&this->device, windowExtents);

		vk::FramebufferAttachmentCreateInfo attachmentCI = {};

		attachmentCI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		attachmentCI.width = deferredShadowFB.width;
		attachmentCI.height = deferredShadowFB.height;
		attachmentCI.layerCount = LIGHT_COUNT;
		attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		deferredShadowFB.AddAttachment(attachmentCI);

		deferredShadowFB.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		deferredShadowFB.CreateRenderPass();

		deferredShadowFB.CreateFramebuffer();
	}

	void DeferredContext::InitializeDescriptors() 
	{
		assert(deferredMRTFB.sampler != VK_NULL_HANDLE);

		const uint32_t num_pipelines = 3;
		/* const uint32_t */
		std::vector<VkDescriptorPoolSize> descriptorPoolSize = 
		{
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  gMaxFramesInFlight * (3)), //1 UB/set * 3 sets -- uniform buffer in deferredMRT.vert, and deferredLightPass.frag
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gMaxFramesInFlight * (4 * 4)) //3 samplers (3 CI/set * 3 sets)-- in composition pipeline, +1 for freddy head texture, +1 for gman head texture.			
		};

		VkDescriptorPoolCreateInfo descriptorPoolCI = vk::init::DescriptorPoolCreateInfo(descriptorPoolSize, (num_pipelines + 2) * gMaxFramesInFlight); //+1 for the freddy head texture, +1 for gman texture

		VK_CHECK_RESULT(vkCreateDescriptorPool(device.logical, &descriptorPoolCI, nullptr, &descriptorPool));
		
		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			//since each uniform is in their own pipeline, just create one layout for binding 0
			vk::init::DescriptorLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				VK_SHADER_STAGE_VERTEX_BIT | //transformUBO
				VK_SHADER_STAGE_FRAGMENT_BIT | //lightUBO
				VK_SHADER_STAGE_GEOMETRY_BIT), //shadowMVP UBO
			vk::init::DescriptorLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), //position
			vk::init::DescriptorLayoutBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), //normal
			vk::init::DescriptorLayoutBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), //UV
			vk::init::DescriptorLayoutBinding(4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //shadow map
		};
		
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vk::init::DescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device.logical, &descriptorSetLayoutCI, nullptr, &this->sceneDescriptorSetLayout));

		VkDescriptorSetAllocateInfo descriptorSetInfo = vk::init::DescriptorSetAllocateInfo(descriptorPool, &this->sceneDescriptorSetLayout, 1);
		
		std::vector<VkDescriptorImageInfo> descriptorImage; //position, normal, and albedo attachments
		for (const auto& attachment : deferredMRTFB.attachments)
		{
			if (attachment.flags & VKC_ATTACHMENT_IS_SAMPLED)
			{
				VkDescriptorImageInfo descInfo = {};
				descInfo.imageLayout = attachment.description.finalLayout;
				descInfo.imageView = attachment.imageView;
				descInfo.sampler = deferredMRTFB.sampler;

				descriptorImage.push_back(descInfo);
			}
		}

		VkDescriptorImageInfo shadowMapDescriptor;
		shadowMapDescriptor.imageLayout = deferredShadowFB.attachments[0].description.finalLayout;
		shadowMapDescriptor.imageView = deferredShadowFB.attachments[0].imageView;
		shadowMapDescriptor.sampler = deferredShadowFB.sampler;

		VkDescriptorImageInfo albedoImageSampler = {};
		albedoImageSampler.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoImageSampler.imageView = defaultTexture.mTextureImageView;
		albedoImageSampler.sampler = deferredMRTFB.sampler; //reusing sampler from special framebuffer, same requirements

		for (uint32_t i = 0; i < descriptorSets.size(); ++i) 
		{
			//deferredMRT descriptor set
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logical, &descriptorSetInfo, &descriptorSets[i].deferred));

			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				vk::init::WriteDescriptorSet(descriptorSets[i].deferred, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].deferredMRT.descriptor),
				vk::init::WriteDescriptorSet(descriptorSets[i].deferred, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &albedoImageSampler)
			};

			//TODO: a little janky way to initialize as more of mInfo is filled with derived classes.
			mInfo.sceneWriteDescriptorSets = { writeDescriptorSets[0] };

			vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

			//deferred l-pass descriptor set
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logical, &descriptorSetInfo, &descriptorSets[i].composition));

			writeDescriptorSets = {
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].composition.descriptor),
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &descriptorImage[RT_POSITION]),
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &descriptorImage[RT_NORMAL]),
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &descriptorImage[RT_ALBEDO]),
				vk::init::WriteDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &shadowMapDescriptor)
			};
			vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

			//deferred shadowing
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logical, &descriptorSetInfo, &descriptorSets[i].deferredShadow));

			writeDescriptorSets = {
				vk::init::WriteDescriptorSet(descriptorSets[i].deferredShadow, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].deferredShadow.descriptor),
			};
			vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
		}

	}
	
	void DeferredContext::InitializePipeline(std::string vsFile, std::string fsFile)
	{
		(void)vsFile;
		(void)fsFile;

		std::vector<VkPushConstantRange> pushConstantRanges = {
			vk::init::PushConstantRange(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
		};

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pSetLayouts = &sceneDescriptorSetLayout;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		VK_CHECK_RESULT(vkCreatePipelineLayout(device.logical, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vk::init::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vk::init::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vk::init::PipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vk::init::PipelineViewportStateCreateInfo(1, 1);
		VkPipelineMultisampleStateCreateInfo multiplesampleStateCI = vk::init::PipelineMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vk::init::PipelineCreateInfo(pipelineLayout, renderPass);
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pMultisampleState = &multiplesampleStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

	
		VkPipelineVertexInputStateCreateInfo emptyVertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyVertexInputStateCI;

		/////////////////////////////////////////////////////////////
		//pipeline #1: lightpass stage of deferred shading
		ShaderModuleInfo vertShaderInfo(device.logical, "deferredLightPass.vert", VK_SHADER_STAGE_VERTEX_BIT);
		ShaderModuleInfo fragShaderInfo(device.logical, "deferredLightPass.frag", VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);

	
		shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
		shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.mHandle, fragShaderInfo.mFlags);
		
		rasterizationStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;


		pipelineManager.AddModule(DeferredPipelines::LIGHTPASS, vertShaderInfo);
		pipelineManager.AddModule(DeferredPipelines::LIGHTPASS, fragShaderInfo);

		VkPipeline lightPassPipeline = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &lightPassPipeline));
		
		//for the hot reloading - light pass 
		std::function<void()> lightPassCreationFunction = 
				[this,
				inputAssemblyStateCI, 
				rasterizationStateCI, 
				depthStencilStateCI,
				multiplesampleStateCI,
				viewportStateCI,
				emptyVertexInputStateCI]
		{

				VkPipeline pipeline = pipelineManager.Get(DeferredPipelines::LIGHTPASS);
				
				if (pipeline != VK_NULL_HANDLE) 
				{
					vkDestroyPipeline(device.logical, pipeline, nullptr);
					pipeline = VK_NULL_HANDLE;
				}

				VkPipelineColorBlendAttachmentState blendAttachmentState = vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
				VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

				std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
				VkPipelineDynamicStateCreateInfo dynamicStateCI = vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

				std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
				
				VkGraphicsPipelineCreateInfo pipelineCI = vk::init::PipelineCreateInfo(pipelineLayout, renderPass);

				pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
				pipelineCI.pRasterizationState = &rasterizationStateCI;
				pipelineCI.pColorBlendState = &colorBlendStateCI;
				pipelineCI.pDepthStencilState = &depthStencilStateCI;
				pipelineCI.pMultisampleState = &multiplesampleStateCI;
				pipelineCI.pDynamicState = &dynamicStateCI;
				pipelineCI.pViewportState = &viewportStateCI;
				pipelineCI.pVertexInputState = &emptyVertexInputStateCI;
				pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
				pipelineCI.pStages = shaderStages.data();

				const std::vector<ShaderModuleInfo>& shaders = pipelineManager.GetPipelineShaders(DeferredPipelines::LIGHTPASS);
				shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(shaders[0].mHandle, shaders[0].mFlags);
				shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(shaders[1].mHandle, shaders[1].mFlags);

				VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

				pipelineManager.AddPipeline(DeferredPipelines::LIGHTPASS, pipeline);
		};


		pipelineManager.AddPipeline(DeferredPipelines::LIGHTPASS, lightPassPipeline, std::move(lightPassCreationFunction));

	
		/////////////////////////////////////////////////////////////
		//pipeline #2: MRT stage of deferred shading -- outputting to color/textures
		vertShaderInfo = ShaderModuleInfo(device.logical, "deferredMRT.vert", VK_SHADER_STAGE_VERTEX_BIT);
		fragShaderInfo = ShaderModuleInfo(device.logical, "deferredMRT.frag", VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);


		shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
		shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.mHandle, fragShaderInfo.mFlags);


		rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;

		pipelineCI.renderPass = deferredMRTFB.renderPass;

		//there are three color outputs in this stage.
		std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
			vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};

		colorBlendStateCI.pAttachments = blendAttachmentStates.data();
		colorBlendStateCI.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());


		//reminder: using a single vertex binding, so binding is 0.
		VkVertexInputBindingDescription vertexBindingDescription = vk::init::VertexInputBindingDescription();
		auto vertexInputAttributeDescriptions = Vertex::InputAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
		vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
		vertexInputStateCI.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();

		pipelineCI.pVertexInputState = &vertexInputStateCI;


		pipelineManager.AddModule(DeferredPipelines::MRT, vertShaderInfo);
		pipelineManager.AddModule(DeferredPipelines::MRT, fragShaderInfo);

		VkPipeline deferredMRTPipeline = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &deferredMRTPipeline));

		//for hot reloading - MRT pass
		std::function<void()> MRTPassCreationFunction =
			[this,
			inputAssemblyStateCI,
			rasterizationStateCI,
			depthStencilStateCI,
			multiplesampleStateCI,
			viewportStateCI]
		{

				VkPipeline pipeline = pipelineManager.Get(DeferredPipelines::MRT);

				if (pipeline != VK_NULL_HANDLE)
				{
					vkDestroyPipeline(device.logical, pipeline, nullptr);
					pipeline = VK_NULL_HANDLE;
				}

				std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
					vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
					vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
					vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE)
				};

				VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vk::init::PipelineColorBlendStateCreateInfo
				(
					blendAttachmentStates.size(), blendAttachmentStates.data()
				);

				std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
				VkPipelineDynamicStateCreateInfo dynamicStateCI = vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

				VkVertexInputBindingDescription vertexBindingDescription = vk::init::VertexInputBindingDescription();
				auto vertexInputAttributeDescriptions = Vertex::InputAttributeDescriptions();

				VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
				vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
				vertexInputStateCI.vertexBindingDescriptionCount = 1;
				vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
				vertexInputStateCI.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();

				std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

				VkGraphicsPipelineCreateInfo pipelineCI = vk::init::PipelineCreateInfo(pipelineLayout, deferredMRTFB.renderPass);

				pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
				pipelineCI.pRasterizationState = &rasterizationStateCI;
				pipelineCI.pColorBlendState = &colorBlendStateCI;
				pipelineCI.pDepthStencilState = &depthStencilStateCI;
				pipelineCI.pMultisampleState = &multiplesampleStateCI;
				pipelineCI.pDynamicState = &dynamicStateCI;
				pipelineCI.pViewportState = &viewportStateCI;
				pipelineCI.pVertexInputState = &vertexInputStateCI;
				pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
				pipelineCI.pStages = shaderStages.data();

				const std::vector<ShaderModuleInfo>& shaders = pipelineManager.GetPipelineShaders(DeferredPipelines::MRT);
				shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(shaders[0].mHandle, shaders[0].mFlags);
				shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(shaders[1].mHandle, shaders[1].mFlags);

				VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

				pipelineManager.AddPipeline(DeferredPipelines::MRT, pipeline);
		};


		pipelineManager.AddPipeline(DeferredPipelines::MRT, deferredMRTPipeline, std::move(MRTPassCreationFunction));

		/////////////////////////////////////////////////////////////
		//pipeline #3: deferred shadow mapping
		vertShaderInfo = ShaderModuleInfo(device.logical, "deferredShadow.vert", VK_SHADER_STAGE_VERTEX_BIT);		
		ShaderModuleInfo geoShaderInfo(device.logical, "deferredShadow.geom", VK_SHADER_STAGE_GEOMETRY_BIT, shaderc_geometry_shader);

		shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
		shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(geoShaderInfo.mHandle, geoShaderInfo.mFlags);

		//shadow pass doesn't have color attachments
		colorBlendStateCI.attachmentCount = 0;
		colorBlendStateCI.pAttachments = nullptr;

		//enable depth bias as a dynamic state
		rasterizationStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizationStateCI.depthBiasEnable = VK_TRUE;

		dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		dynamicStateCI = vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

		pipelineCI.renderPass = deferredShadowFB.renderPass;
		
		pipelineManager.AddModule(DeferredPipelines::SHADOW, vertShaderInfo);
		pipelineManager.AddModule(DeferredPipelines::SHADOW, geoShaderInfo);

		VkPipeline deferredShadowPipeline = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &deferredShadowPipeline));

		pipelineManager.AddPipeline(DeferredPipelines::SHADOW, deferredShadowPipeline, nullptr);

	}

	void DeferredContext::RecordCommandBuffers()
	{
		ObjectManager& objManager = _Application->ObjectManager();

		DescriptorSets currentDescriptorSet = descriptorSets[currentFrame];

		VkCommandBuffer cmdBuffer = commandBuffers[currentFrame];
		VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();

		//clear value count corresponds to the number of attachments.
		VkClearValue clearValues[4]; //position, normal, albedo, depth;

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));

		//deferred shadow rendering
		{
			clearValues[0].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBI = vk::init::RenderPassBeginInfo();
			renderPassBI.clearValueCount = 1;
			renderPassBI.pClearValues = clearValues;
			renderPassBI.renderArea.extent = { (uint32_t)deferredShadowFB.width, (uint32_t)deferredShadowFB.height };
			renderPassBI.renderPass = deferredShadowFB.renderPass;
			renderPassBI.framebuffer = deferredShadowFB.handle;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport deferredShadowViewport = vk::init::Viewport(deferredShadowFB.width, deferredShadowFB.height);
			vkCmdSetViewport(cmdBuffer, 0, 1, &deferredShadowViewport);

			VkRect2D deferredShadowScissor = vk::init::Rect2D(deferredShadowFB.width, deferredShadowFB.height);
			vkCmdSetScissor(cmdBuffer, 0, 1, &deferredShadowScissor);

			vkCmdSetDepthBias(cmdBuffer, depthBiasConstant, 0.f, depthBiasSlope);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.Get(DeferredPipelines::SHADOW));

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &currentDescriptorSet.deferredShadow, 0, nullptr);

			objManager.DrawObjects(cmdBuffer, pipelineLayout);

			vkCmdEndRenderPass(cmdBuffer);
		}


		//MRT rendering.
		{
			clearValues[0].color = { 0,0,0,0 };
			clearValues[1].color = clearValues[0].color;
			clearValues[2].color = clearValues[0].color;
			clearValues[3].depthStencil = { 1.f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = 4;
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.renderArea.extent = { (uint32_t)deferredMRTFB.width, (uint32_t)deferredMRTFB.height };
			renderPassBeginInfo.renderPass = deferredMRTFB.renderPass;
			renderPassBeginInfo.framebuffer = deferredMRTFB.handle;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport deferredMRTViewport = vk::init::Viewport(deferredMRTFB.width, deferredMRTFB.height);
			vkCmdSetViewport(cmdBuffer, 0, 1, &deferredMRTViewport);

			VkRect2D deferredMRTScissor = vk::init::Rect2D(deferredMRTFB.width, deferredMRTFB.height);
			vkCmdSetScissor(cmdBuffer, 0, 1, &deferredMRTScissor);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.Get(DeferredPipelines::MRT));

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &currentDescriptorSet.deferred, 0, nullptr);

			objManager.DrawObjects(cmdBuffer, pipelineLayout);

			vkCmdEndRenderPass(cmdBuffer);
		}

		//light pass rendering - composition
		{
			clearValues[0].color = { 0,0,0,0 };
			clearValues[1].depthStencil = { 1.f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.renderArea.extent = {(uint32_t)window.viewport.width, (uint32_t)window.viewport.height};
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = swapChain.framebuffers[currentFrame].handle;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport = window.viewport;
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = window.scissor;
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &currentDescriptorSet.composition, 0, nullptr);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.Get(DeferredPipelines::LIGHTPASS));

			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			if (settings.UIEnabled) 
			{
				UIOverlay.Render(cmdBuffer);
			}

			vkCmdEndRenderPass(cmdBuffer);

		}

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

	}

	void DeferredContext::UpdateUI() 
	{
		static bool option = false;
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

	void DeferredContext::Render() 
	{
		pipelineManager.HotReloadShaders();

		if (window.isPrepared) 
		{
			ContextBase::PrepareFrame();
			UpdateScreenUniforms();
			UpdateSceneUniforms();
			RecordCommandBuffers();
			ContextBase::SubmitFrame();
		}
	}

}