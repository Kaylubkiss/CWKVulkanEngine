#include "vkDeferredShadingContext.h"

namespace vk 
{

	DeferredContext::DeferredContext() 
	{

		deferredPass.width = window.viewport.width;
		deferredPass.height = window.viewport.height;

		defaultTexture = Texture(&this->mInfo, "wood-floor.png");
		UIOverlay.AddImage(defaultTexture);

		DeferredContext::InitializeUniforms();
		DeferredContext::IntializeDeferredFramebuffer();
		DeferredContext::IntializeColorSampler();
		DeferredContext::InitializeDescriptors();
		DeferredContext::InitializePipeline();

		FillOutGraphicsContextInfo();
		
	}


	DeferredContext::~DeferredContext() 
	{
		uniformBuffers.deferredMRT.UnMap();
		uniformBuffers.deferredMRT.Destroy();
		
		uniformBuffers.deferredLightPass.UnMap();
		uniformBuffers.deferredLightPass.Destroy();

		defaultTexture.Destroy(device.logical);

		vkDestroyRenderPass(device.logical, deferredPass.renderPass, nullptr);
		vkDestroyFramebuffer(device.logical, deferredPass.framebuffer, nullptr);
		vkDestroySampler(device.logical, colorSampler, nullptr);

		vkDestroyDescriptorSetLayout(device.logical, this->sceneDescriptorSetLayout, nullptr);
		vkDestroyPipeline(device.logical, deferredMRTPipeline, nullptr);
	}

	void DeferredContext::InitializeScene(ObjectManager& objManager) 
	{
		glm::mat4 modelTransform = glm::mat4(5.f);
		modelTransform[3] = glm::vec4(1.0f, 0, 5.f, 1);


		ObjectCreateInfo objectCI;
		objectCI.objName = "freddy.obj";
		objectCI.textureFileName = "myFace.jpg";
		objectCI.pModelTransform = &modelTransform;

		objManager.LoadObject(objectCI);

		//object 2
		modelTransform = glm::mat4(1.f);
		modelTransform[3] = glm::vec4(0, 20, -5.f, 1);

		PhysicsComponent physicsComponent;
		physicsComponent.bodyType = BodyType::DYNAMIC;
		physicsComponent.colliderType = PhysicsComponent::ColliderType::CUBE;

		objectCI = {};
		objectCI.objName = "cube.obj";
		objectCI.textureFileName = "";
		objectCI.pPhysicsComponent = &physicsComponent;
		objectCI.pModelTransform = &modelTransform;

		objManager.LoadObject(objectCI);

		//object 3
		const float dbScale = 30.f;
		modelTransform = glm::mat4(dbScale);
		modelTransform[3] = { 0.f, -5.f, 0.f, 1 };

		physicsComponent.bodyType = reactphysics3d::BodyType::STATIC;


		objectCI = {};
		objectCI.objName = "base.obj";
		objectCI.textureFileName = "";
		objectCI.pPhysicsComponent = &physicsComponent;
		objectCI.pModelTransform = &modelTransform;

		objManager.LoadObject(objectCI);

	}

	void DeferredContext::ResizeWindow() 
	{
		ContextBase::ResizeWindow();
		deferredPass.width = window.viewport.width;
		deferredPass.height = window.viewport.height;
		
		
		vkDestroyRenderPass(device.logical, deferredPass.renderPass, nullptr);
		vkDestroyFramebuffer(device.logical, deferredPass.framebuffer, nullptr);

		deferredPass.position.Destroy(device.logical);
		deferredPass.normal.Destroy(device.logical);
		deferredPass.albedo.Destroy(device.logical);
		deferredPass.depth.Destroy(device.logical);

		DeferredContext::IntializeDeferredFramebuffer();

		//must also update the descriptors as they are still pointing to the old image views.
		std::array<VkDescriptorImageInfo, 3> descriptorImage;

		//position descriptor
		descriptorImage[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImage[0].imageView = deferredPass.position.imageView;
		descriptorImage[0].sampler = colorSampler;

		//normal descriptor
		descriptorImage[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImage[1].imageView = deferredPass.normal.imageView;
		descriptorImage[1].sampler = colorSampler;

		//albedo
		descriptorImage[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImage[2].imageView = deferredPass.albedo.imageView;
		descriptorImage[2].sampler = colorSampler;

		std::vector<VkWriteDescriptorSet>writeDescriptorSets = {
			vk::init::WriteDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &descriptorImage[0]),
			vk::init::WriteDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &descriptorImage[1]),
			vk::init::WriteDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &descriptorImage[2])
		};
		vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

		
	}

	void DeferredContext::FillOutGraphicsContextInfo() 
	{
		mInfo.descriptorPool = this->descriptorPool;
		mInfo.descriptorSetLayout = this->sceneDescriptorSetLayout;
		mInfo.samplerBinding = 3;

		//fill out the rest of the struct.
		ContextBase::FillOutGraphicsContextInfo();
	}

	void DeferredContext::InitializeDeferredRenderPass()
	{
		VkRenderPassCreateInfo createInfo = vk::init::RenderPassCreateInfo();


		std::array<VkAttachmentDescription, 4> attachmentDesc = {};

		for (int i = 0; i < attachmentDesc.size(); ++i) 
		{
			attachmentDesc[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDesc[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDesc[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDesc[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDesc[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDesc[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			if (i == 3) 
			{
				attachmentDesc[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; //the depth won't be read from in the composition pass
			}
			else 
			{
				attachmentDesc[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //cause this will be read by the contextbase's renderpass.
			}
		}

		attachmentDesc[0].format = deferredPass.position.format;
		attachmentDesc[1].format = deferredPass.normal.format;
		attachmentDesc[2].format = deferredPass.albedo.format;
		attachmentDesc[3].format = deferredPass.depth.format;

		std::array<VkAttachmentReference, 3> colorReferences;
		colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; //position
		colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; //normals
		colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; //albedo

		VkAttachmentReference depthReference = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };


		std::array<VkSubpassDependency,3> dependencies = {};
		
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;



		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		//all of the memory reads needs to be done. We're just going to overwrite whatever was written so don't need to "oversynchronize" 
		dependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[2].srcSubpass = 0;
		dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		//we're waiting for all reads and writes to be completed (since the l-buffer will be reading the color attachments, 
		// and these color attachments also need to be written to prior).
		dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT; //next subpass will then take these color attachments and finally render them.
		

		VkSubpassDescription subpass = {};
		subpass.colorAttachmentCount = colorReferences.size();
		subpass.pColorAttachments = colorReferences.data();
		subpass.pDepthStencilAttachment = &depthReference;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		createInfo.pSubpasses = &subpass;
		createInfo.subpassCount = 1;
		createInfo.pAttachments = attachmentDesc.data();
		createInfo.attachmentCount = attachmentDesc.size();
		createInfo.pDependencies = dependencies.data();
		createInfo.dependencyCount = dependencies.size();


		VK_CHECK_RESULT(vkCreateRenderPass(device.logical, &createInfo, nullptr, &deferredPass.renderPass));
	}

	void DeferredContext::IntializeColorSampler() 
	{
		VkSamplerCreateInfo samplerCI = vk::init::SamplerCreateInfo();
		samplerCI.magFilter = VK_FILTER_LINEAR;
		samplerCI.minFilter = VK_FILTER_LINEAR;
		samplerCI.mipLodBias = 0.f;
		samplerCI.minLod = 0.0f;
		samplerCI.maxLod = 1.0f;
		samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.addressModeV = samplerCI.addressModeU;
		samplerCI.addressModeW = samplerCI.addressModeU;
		samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VK_CHECK_RESULT(vkCreateSampler(device.logical, &samplerCI, nullptr, &colorSampler));

	}

	void DeferredContext::InitializeUniforms() 
	{
		uniformBuffers.deferredMRT = device.CreateBuffer(sizeof(uniformDataMRT), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)(&uniformDataMRT));
		uniformBuffers.deferredMRT.Map();

		//TODO: stationary light. single light.
		uniformDataLightPass.light.pos = {0,10, 10};
		uniformDataLightPass.light.albedo = { 1.0, 1.0, 1.0 };
		uniformDataLightPass.light.ambient = uniformDataLightPass.light.albedo * 0.1f;
		uniformDataLightPass.light.specular = { 0.5f, 0.5f, 0.5f };
		uniformDataLightPass.light.shininess = 32.f;

		uniformBuffers.deferredLightPass = device.CreateBuffer(sizeof(uniformDataLightPass), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)(&uniformDataLightPass));
		uniformBuffers.deferredLightPass.Map(); 
		
	}

	void DeferredContext::UpdateScreenUniforms()
	{
		//transform(s)
		if (settings.minimized == false) 
		{
			uniformDataMRT.uTransform =
			{
				mCamera.LookAt(),
				glm::perspective(glm::radians(FOV), (float)window.viewport.width / window.viewport.height, 0.1f, 1000.f)
			};

			uniformDataMRT.uTransform.proj[1][1] *= -1;

			memcpy(uniformBuffers.deferredMRT.mappedMemory, (void*)(&uniformDataMRT), sizeof(uniformDataMRT));
		}
		
	}

	void DeferredContext::UpdateSceneUniforms() 
	{
		//light(s)
		uniformDataLightPass.viewPosition = mCamera.Position();

		memcpy(uniformBuffers.deferredLightPass.mappedMemory, (void*)(&uniformDataLightPass), sizeof(uniformDataLightPass));
	}

	void DeferredContext::IntializeDeferredFramebuffer() 
	{
		VkFramebufferCreateInfo framebuffer = vk::init::FramebufferCreateInfo();
		framebuffer.width = deferredPass.width;
		framebuffer.height = deferredPass.height;
		framebuffer.layers = 1;
		
		deferredPass.position = device.CreateFramebufferAttachment(window.viewport, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R16G16B16A16_SFLOAT);
		deferredPass.normal = device.CreateFramebufferAttachment(window.viewport, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R16G16B16A16_SFLOAT);
		deferredPass.albedo = device.CreateFramebufferAttachment(window.viewport, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_UNORM);

		deferredPass.depth = device.CreateFramebufferAttachment(window.viewport, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		std::array<VkImageView, 4> attachments =
		{
			deferredPass.position.imageView, 
			deferredPass.normal.imageView,
			deferredPass.albedo.imageView,
			deferredPass.depth.imageView
		};

		framebuffer.pAttachments = attachments.data();
		framebuffer.attachmentCount = attachments.size();
		
		DeferredContext::InitializeDeferredRenderPass();
		framebuffer.renderPass = deferredPass.renderPass;

		
		VK_CHECK_RESULT(vkCreateFramebuffer(this->device.logical, &framebuffer, nullptr, &deferredPass.framebuffer));

	}

	void DeferredContext::InitializeDescriptors() 
	{
		assert(colorSampler != VK_NULL_HANDLE);

		//NOTE: non-textured objects in this scene
		const uint32_t num_pipelines = 2;
		std::vector<VkDescriptorPoolSize> descriptorPoolSize = {
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2), //uniform buffer in deferredMRT.vert, and deferredLightPass.frag
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4) //3 samplers in composition pipeline, +1 for freddy head texture.			
		};

		VkDescriptorPoolCreateInfo descriptorPoolCI = vk::init::DescriptorPoolCreateInfo(descriptorPoolSize, num_pipelines + 1); //+1 for the freddy head texture.
		VK_CHECK_RESULT(vkCreateDescriptorPool(device.logical, &descriptorPoolCI, nullptr, &descriptorPool));


		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			vk::init::DescriptorLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT), //transformUBO
			vk::init::DescriptorLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), //position
			vk::init::DescriptorLayoutBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), //normal
			vk::init::DescriptorLayoutBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), //UV
			vk::init::DescriptorLayoutBinding(4, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) //lightUBO
			//might wanna add a uniform for light later...
		};
		
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vk::init::DescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device.logical, &descriptorSetLayoutCI, nullptr, &this->sceneDescriptorSetLayout));

		VkDescriptorSetAllocateInfo descriptorSetInfo = vk::init::DescriptorSetAllocateInfo(descriptorPool, &this->sceneDescriptorSetLayout, 1);
		
		//deferredMRT descriptor set
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logical, &descriptorSetInfo, &descriptorSets.deferred));

		VkDescriptorImageInfo albedoImageSampler = {};
		albedoImageSampler.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoImageSampler.imageView = defaultTexture.mTextureImageView;
		albedoImageSampler.sampler = colorSampler;

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vk::init::WriteDescriptorSet(descriptorSets.deferred, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.deferredMRT.descriptor),
			vk::init::WriteDescriptorSet(descriptorSets.deferred, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &albedoImageSampler)
		};

		//TODO: a little janky way to initialize as more of mInfo is filled with derived classes.
		mInfo.sceneWriteDescriptorSets = { writeDescriptorSets[0] };

		vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
		
		//deferred l-pass descriptor set
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logical, &descriptorSetInfo, &descriptorSets.composition));

		std::array<VkDescriptorImageInfo, 3> descriptorImage;
			
		//position descriptor
		descriptorImage[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImage[0].imageView = deferredPass.position.imageView;
		descriptorImage[0].sampler = colorSampler;

		//normal descriptor
		descriptorImage[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImage[1].imageView = deferredPass.normal.imageView;
		descriptorImage[1].sampler = colorSampler;

		descriptorImage[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImage[2].imageView = deferredPass.albedo.imageView;
		descriptorImage[2].sampler = colorSampler;


		writeDescriptorSets = {
			vk::init::WriteDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &descriptorImage[0]),
			vk::init::WriteDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &descriptorImage[1]),
			vk::init::WriteDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &descriptorImage[2]),
			vk::init::WriteDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, &uniformBuffers.deferredLightPass.descriptor)
		};
		vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
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
		VK_CHECK_RESULT(vkCreatePipelineLayout(device.logical, &pipelineLayoutCreateInfo, nullptr, &this->mPipeline.layout));

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

		VkGraphicsPipelineCreateInfo pipelineCI = vk::init::PipelineCreateInfo(this->mPipeline.layout, this->mPipeline.mRenderPass);
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
		mPipeline.AddModule(vertShaderInfo).AddModule(fragShaderInfo);	//just for memory management...


		shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
		shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.mHandle, fragShaderInfo.mFlags);


		rasterizationStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &mPipeline.handle));

		/////////////////////////////////////////////////////////////
		//pipeline #2: MRT stage of deferred shading -- outputting to color/textures
		vertShaderInfo = ShaderModuleInfo(device.logical, "deferredMRT.vert", VK_SHADER_STAGE_VERTEX_BIT);
		fragShaderInfo = ShaderModuleInfo(device.logical, "deferredMRT.frag", VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);
		mPipeline.AddModule(vertShaderInfo).AddModule(fragShaderInfo); //for memory management purposes

		shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
		shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.mHandle, fragShaderInfo.mFlags);


		rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;

		pipelineCI.renderPass = deferredPass.renderPass;

		//there are three color outputs in this stage.
		std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
			vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};

		colorBlendStateCI.pAttachments = blendAttachmentStates.data();
		colorBlendStateCI.attachmentCount = blendAttachmentStates.size();


		//reminder: using a single vertex binding, so binding is 0.
		VkVertexInputBindingDescription vertexBindingDescription = vk::init::VertexInputBindingDescription();
		auto vertexInputAttributeDescriptions = Vertex::InputAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
		vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
		vertexInputStateCI.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();

		pipelineCI.pVertexInputState = &vertexInputStateCI;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &deferredMRTPipeline));
	}

	void DeferredContext::RecordCommandBuffers()
	{
		if (settings.minimized == false) 
		{
			ObjectManager& objManager = _Application->ObjectManager();

			VkCommandBuffer cmdBuffer = commandBuffers[currentFrame];
			VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();

			//clear value count corresponds to the number of attachments.
			VkClearValue clearValues[4]; //position, normal, albedo, depth;

			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));

			//MRT rendering.
			{
				clearValues[0].color = { 0,0,0,0 };
				clearValues[1].color = clearValues[0].color;
				clearValues[2].color = clearValues[0].color;
				clearValues[3].depthStencil = { 1.f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
				renderPassBeginInfo.clearValueCount = 4;
				renderPassBeginInfo.pClearValues = clearValues;
				renderPassBeginInfo.renderArea.extent = { (uint32_t)deferredPass.width, (uint32_t)deferredPass.height };
				renderPassBeginInfo.renderPass = deferredPass.renderPass;
				renderPassBeginInfo.framebuffer = deferredPass.framebuffer;

				vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport deferredMRTViewport = vk::init::Viewport(deferredPass.width, deferredPass.height);
				vkCmdSetViewport(cmdBuffer, 0, 1, &deferredMRTViewport);

				VkRect2D deferredMRTScissor = vk::init::Rect2D(deferredPass.width, deferredPass.height);
				vkCmdSetScissor(cmdBuffer, 0, 1, &deferredMRTScissor);

				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredMRTPipeline);

				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.layout, 0, 1, &descriptorSets.deferred, 0, nullptr);

				objManager.DrawObjects(cmdBuffer, mPipeline.layout);

				vkCmdEndRenderPass(cmdBuffer);
			}

			//light pass rendering
			{
				clearValues[0].color = { 0,0,0,0 };
				clearValues[1].depthStencil = { 1.f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
				renderPassBeginInfo.clearValueCount = 4;
				renderPassBeginInfo.pClearValues = clearValues;
				renderPassBeginInfo.renderArea.extent = {(uint32_t)window.viewport.width, (uint32_t)window.viewport.height};
				renderPassBeginInfo.renderPass = mPipeline.mRenderPass;
				renderPassBeginInfo.framebuffer = swapChain.frameBuffers[currentFrame];

				vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport sceneViewport = window.viewport;
				vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

				VkRect2D sceneScissor = window.scissor;
				vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.layout, 0, 1, &descriptorSets.composition, 0, nullptr);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.handle);
				vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

				UIOverlay.Render(cmdBuffer); //TODO: fix the recording of this. Seems to cause queuesubmit some trouble.

				vkCmdEndRenderPass(cmdBuffer);

			}


			VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
		}

	}

	void DeferredContext::UpdateUI() 
	{
		static bool option = false;
		if (UIOverlay.CollapsingHeader("Deferred Context Settings"))
		{
			UIOverlay.CheckBox("box test", &option);
			UIOverlay.SeparatorText("light position");
			UIOverlay.Slider("", uniformDataLightPass.light.pos);
			UIOverlay.SeparatorText("textures in scene");
			UIOverlay.DisplayImages();
		}
	}

	void DeferredContext::Render() 
	{
		UpdateSceneUniforms();
		if (window.IsMinimized() == false) 
		{
			ContextBase::PrepareFrame();
			UpdateScreenUniforms();
			RecordCommandBuffers();
			ContextBase::SubmitFrame();
		}
	}

}