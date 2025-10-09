#include "ApplicationGlobal.h"
#include "vkShadowMapContext.h"


namespace vk 
{
	ShadowMapScene::ShadowMapScene() 
	{
		std::cout << "this scene is still under construction due to core rewrites, and will probably crash : )\n";
		_Application->RequestExit();
		return;

		uLightObject& light = uniformDataScene.light;

		light.pos = { 0,10, 10 };
		light.albedo = { 1.0, 1.0, 1.0 };
		light.ambient = light.albedo * 0.1f;
		light.specular = { 0.5f, 0.5f, 0.5f };
		light.shininess = 32.f;

		this->uniformBuffers.scene = device.CreateBuffer(sizeof(UniformDataScene), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)(&this->uniformDataScene));
		uniformBuffers.scene.Map();

		this->uniformBuffers.offscreen = device.CreateBuffer(sizeof(UniformDataOffscreen), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)(&this->uniformDataOffscreen));
		uniformBuffers.offscreen.Map();

				
		offscreenPass.depth.format = VK_FORMAT_D16_UNORM;

		ShadowMapScene::UpdateOffscreenUniforms();
		ShadowMapScene::UpdateSceneUniforms();

		InitializeOffscreenFramebuffer();
		InitializeDescriptors();
		InitializePipeline();

		FillOutGraphicsContextInfo();
	}

	ShadowMapScene::~ShadowMapScene() 
	{
		offscreenPass.depth.Destroy(device.logical);

		uniformBuffers.offscreen.UnMap();
		uniformBuffers.scene.UnMap();
		this->uniformBuffers.offscreen.Destroy();
		this->uniformBuffers.scene.Destroy();

		vkDestroyRenderPass(device.logical, offscreenPass.renderPass, nullptr);
		vkDestroyFramebuffer(device.logical, offscreenPass.frameBuffer, nullptr);
		vkDestroySampler(device.logical, offscreenPass.depthSampler, nullptr);
	}

	void ShadowMapScene::UpdateSceneUniforms() 
	{
		uniformDataScene.transform = 
		{
			mCamera.LookAt(),
			glm::perspective(glm::radians(45.f), (float)window.viewport.width / window.viewport.height, 0.1f, 1000.f) 
		};
		uniformDataScene.transform.proj[1][1] *= -1.f;
		uniformDataScene.depthBiasMVP = uniformDataOffscreen.depthVP;

		uniformDataScene.camPos = mCamera.Position();
		memcpy(uniformBuffers.scene.mappedMemory, (void*)(&uniformDataScene), sizeof(uniformDataScene));
	}

	void ShadowMapScene::UpdateOffscreenUniforms() 
	{
		glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.f, zNear, zFar);
		depthProjectionMatrix[1][1] *= -1;
		glm::mat4 depthViewMatrix = glm::lookAt(uniformDataScene.light.pos, glm::vec3(0.f), glm::vec3(0, 1, 0));
		uniformDataOffscreen.depthVP = depthProjectionMatrix * depthViewMatrix;

		memcpy(uniformBuffers.offscreen.mappedMemory, &uniformDataOffscreen, sizeof(uniformDataOffscreen));
	}

	void ShadowMapScene::RecordCommandBuffers() 
	{
		/*vk::ObjectManager& objManager = _Application->ObjectManager();

		VkCommandBuffer cmdBuffer = commandBuffers[currentFrame];
		VkCommandBufferBeginInfo cmdBeginInfo = vk::init::CommandBufferBeginInfo();

		VkClearValue clearValue[2];
		VkViewport viewport;
		VkRect2D scissor;

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo));

		{
			clearValue[0].depthStencil = {1.f, 0};

			VkRenderPassBeginInfo offscreenRenderPassInfo = vk::init::RenderPassBeginInfo();
			offscreenRenderPassInfo.framebuffer = offscreenPass.frameBuffer;
			offscreenRenderPassInfo.renderPass = offscreenPass.renderPass;
			offscreenRenderPassInfo.renderArea.extent = {offscreenPass.width, offscreenPass.height};
			offscreenRenderPassInfo.clearValueCount = 1;
			offscreenRenderPassInfo.pClearValues = clearValue;

			vkCmdBeginRenderPass(cmdBuffer, &offscreenRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport offscreenViewport = vk::init::Viewport(offscreenPass.width, offscreenPass.height);
			vkCmdSetViewport(cmdBuffer, 0, 1, &offscreenViewport);

			VkRect2D offscreenScissor = vk::init::Rect2D(offscreenPass.width, offscreenPass.height);
			vkCmdSetScissor(cmdBuffer, 0, 1, &offscreenScissor);

			vkCmdSetDepthBias(cmdBuffer,
				depthBiasConstant, 0.f, 
				depthBiasSlope);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.Get(Pipelines::OFFSCREEN));

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.offscreen, 0, nullptr);

			objManager.DrawObjects(cmdBuffer, pipelineLayout);

			vkCmdEndRenderPass(cmdBuffer);

		}

		{

			clearValue[0].color = { 0.025, 0.025, 0.025, 1.f };
			clearValue[1].depthStencil = {1.f, 0};

			VkRenderPassBeginInfo sceneRenderPassInfo = vk::init::RenderPassBeginInfo();
			sceneRenderPassInfo.framebuffer = swapChain.frameBuffers[currentFrame];
			sceneRenderPassInfo.renderPass = renderPass;
			sceneRenderPassInfo.renderArea.extent = {(uint32_t)window.viewport.width, (uint32_t)window.viewport.height};
			sceneRenderPassInfo.clearValueCount = 2;
			sceneRenderPassInfo.pClearValues = clearValue;

			vkCmdBeginRenderPass(cmdBuffer, &sceneRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport = window.viewport;
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = window.scissor;
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);


			if (!showDebug) {
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.Get(Pipelines::FORWARD_RENDER));
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.scene, 0, nullptr);
				objManager.DrawObjects(cmdBuffer, pipelineLayout);
			}
			else 
			{
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.offscreenDebug, 0,nullptr);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenDebugPipeline);
				vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
			}

			vkCmdEndRenderPass(cmdBuffer);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));*/

	}

	void ShadowMapScene::InitializeScene(ObjectManager& objManager) 
	{
		glm::mat4 modelTransform = glm::mat4(5.f);
		modelTransform[3] = glm::vec4(1.0f, 0, 5.f, 1);


		ObjectCreateInfo objectCI;
		objectCI.objName = "freddy.obj";
		objectCI.textureFileName = "";
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

	void ShadowMapScene::InitializePipeline(std::string vsFile, std::string fsFile)
	{

		//(void)vsFile;
		//(void)fsFile;

		//std::vector<VkPushConstantRange> pushConstantRanges = {
		//	vk::init::PushConstantRange(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
		//};

		//VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
		//pipelineLayoutCreateInfo.pSetLayouts = &sceneDescriptorLayout;
		//pipelineLayoutCreateInfo.setLayoutCount = 1;
		//pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
		//pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		//VK_CHECK_RESULT(vkCreatePipelineLayout(device.logical, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		//VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vk::init::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		//VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vk::init::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		//VkPipelineColorBlendAttachmentState blendAttachmentState = vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
		//VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		//VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vk::init::PipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		//VkPipelineViewportStateCreateInfo viewportStateCI = vk::init::PipelineViewportStateCreateInfo(1,1);
		//VkPipelineMultisampleStateCreateInfo multiplesampleStateCI = vk::init::PipelineMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
		//std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		//VkPipelineDynamicStateCreateInfo dynamicStateCI = vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

		//std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		//VkGraphicsPipelineCreateInfo pipelineCI = vk::init::PipelineCreateInfo(pipelineLayout, renderPass);
		//pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		//pipelineCI.pRasterizationState = &rasterizationStateCI;
		//pipelineCI.pColorBlendState = &colorBlendStateCI;
		//pipelineCI.pDepthStencilState = &depthStencilStateCI;
		//pipelineCI.pMultisampleState = &multiplesampleStateCI;
		//pipelineCI.pDynamicState = &dynamicStateCI;
		//pipelineCI.pViewportState = &viewportStateCI;
		//pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		//pipelineCI.pStages = shaderStages.data();

		////TODO: make this more extensible. 
		//VkVertexInputBindingDescription vertexBindingDescription = vk::init::VertexInputBindingDescription();
		//auto vertexInputAttributeDescriptions = Vertex::InputAttributeDescriptions();

		//VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
		//vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
		//vertexInputStateCI.vertexBindingDescriptionCount = 1;
		//vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
		//vertexInputStateCI.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();
		//
		//pipelineCI.pVertexInputState = &vertexInputStateCI;
		//pipelineCI.renderPass = mPipeline.mRenderPass;

		//////////////////////////////////////
		////pipline #1: scene rendering pipeline with shadows applied, no filter
		//ShaderModuleInfo vertShaderInfo(device.logical, "sceneShadowMap.vert", VK_SHADER_STAGE_VERTEX_BIT);
		//ShaderModuleInfo fragShaderInfo(device.logical, "sceneShadowMap.frag", VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);
		//mPipeline.AddModule(vertShaderInfo).AddModule(fragShaderInfo);	//just for memory management...

		//shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
		//shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.mHandle, fragShaderInfo.mFlags);
		//
		////pSpecializationInfo on the fragment stage for filtering toggle...?
		//VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &mPipeline.handle));

		//////////////////////////
		////pipeline #2: debug screen for shadow mapping
		//rasterizationStateCI.cullMode = VK_CULL_MODE_NONE; //make sure all faces contribute to shadow rendering.

		//VkPipelineVertexInputStateCreateInfo emptyVertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
		//pipelineCI.pVertexInputState = &emptyVertexInputStateCI;


		//vertShaderInfo = ShaderModuleInfo(device.logical, "debugShadowMap.vert", VK_SHADER_STAGE_VERTEX_BIT);
		//fragShaderInfo = ShaderModuleInfo(device.logical, "debugShadowMap.frag", VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);
		//pipelineManager.AddModule(Pipelines::OFFSCREEN_DEBUG, vertShaderInfo);
		//pipelineManager.AddModule(Pipelines::OFFSCREEN_DEBUG, fragShaderInfo); //for memory management purposes

		//shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
		//shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.mHandle, fragShaderInfo.mFlags);

		//VkPipeline offscreenDebugPipeline = VK_NULL_HANDLE;
		//VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &offscreenDebugPipeline));
		//pipelineManager.AddPipeline(Pipelines::OFFSCREEN_DEBUG, offscreenDebugPipeline);

		///////////////////////////////
		////pipline #3: shadow scene rendering (only need the vertex processing stage)
		//vertShaderInfo = ShaderModuleInfo(device.logical, "offscreenShadowMap.vert", VK_SHADER_STAGE_VERTEX_BIT);
		//pipelineManager.AddModule(Pipelines::OFFSCREEN, vertShaderInfo);

		//shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
		//pipelineCI.stageCount = 1;
		////no color attachment.
		//colorBlendStateCI.attachmentCount = 0;				
		//depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		////enable depth bias, which is used to combat z-fighting by offsetting all the fragments
		////generated through rasterization.
		//rasterizationStateCI.depthBiasEnable = VK_TRUE;		
		//dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		//dynamicStateCI = vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

		//pipelineCI.pVertexInputState = &vertexInputStateCI;
		//pipelineCI.renderPass = offscreenPass.renderPass;
		//VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &offscreenPipeline));

		//

	}

	void ShadowMapScene::InitializeOffscreenRenderPass() 
	{
		VkAttachmentDescription attachment = {};
		attachment.format = offscreenPass.depth.format;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//we won't be using a stencil pass so we don't care.
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 0;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 0;
		subpass.pDepthStencilAttachment = &depthReference;

		//for image layout transitions, we specify the subpass dependencies.
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPass = vk::init::RenderPassCreateInfo();
		renderPass.attachmentCount = 1;
		renderPass.pAttachments = &attachment;
		renderPass.subpassCount = 1;
		renderPass.pSubpasses = &subpass;
		renderPass.dependencyCount = dependencies.size();
		renderPass.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device.logical, &renderPass, nullptr, &offscreenPass.renderPass));
	}

	void ShadowMapScene::InitializeDescriptors() 
	{
		const uint32_t num_shaders = 3;

		std::vector<VkDescriptorPoolSize> poolSizes = {
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, num_shaders),
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_shaders)
		};
		VkDescriptorPoolCreateInfo poolCreateInfo = vk::init::DescriptorPoolCreateInfo(poolSizes, num_shaders);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device.logical, &poolCreateInfo, nullptr, &this->descriptorPool));

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBinding = 
		{
			vk::init::DescriptorLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
			//sampler (shadow map)
			vk::init::DescriptorLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		};
		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = vk::init::DescriptorSetLayoutCreateInfo(setLayoutBinding);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device.logical, &layoutCreateInfo, nullptr, &sceneDescriptorLayout));


		VkDescriptorImageInfo shadowMapDescriptor = {};
		shadowMapDescriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		shadowMapDescriptor.imageView = offscreenPass.depth.imageView;
		shadowMapDescriptor.sampler = offscreenPass.depthSampler;

		//offscreen rendering
		VkDescriptorSetAllocateInfo allocInfo = vk::init::DescriptorSetAllocateInfo(descriptorPool, &sceneDescriptorLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logical, &allocInfo, &descriptorSets.offscreen));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vk::init::WriteDescriptorSet(descriptorSets.offscreen, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.offscreen.descriptor)
		};

		vkUpdateDescriptorSets(device.logical, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

		//offscreen render debug 
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logical, &allocInfo, &descriptorSets.offscreenDebug));
		writeDescriptorSets = {
			vk::init::WriteDescriptorSet(descriptorSets.offscreenDebug, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.scene.descriptor),
			vk::init::WriteDescriptorSet(descriptorSets.offscreenDebug, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &shadowMapDescriptor)
		};
		vkUpdateDescriptorSets(device.logical, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

		//scene descriptor with shadow applied
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logical, &allocInfo, &descriptorSets.scene));
		writeDescriptorSets = {
			//uniform transforms...
			vk::init::WriteDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.scene.descriptor),
			// object texture...?
			//shadow map texture
			vk::init::WriteDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &shadowMapDescriptor)
		};

		//TODO: janky mInfo....
		mInfo.sceneWriteDescriptorSets = writeDescriptorSets;

		vkUpdateDescriptorSets(device.logical, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
	}

	void ShadowMapScene::FillOutGraphicsContextInfo() 
	{
		mInfo.descriptorPool = this->descriptorPool;
		mInfo.descriptorSetLayout = this->sceneDescriptorLayout;
		mInfo.samplerBinding = 2;
		ContextBase::FillOutGraphicsContextInfo();

	}

	void ShadowMapScene::InitializeOffscreenFramebuffer() 
	{
		offscreenPass.width = shadowMapSize;
		offscreenPass.height = shadowMapSize;

		VkImageCreateInfo image = vk::init::ImageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.extent.width = offscreenPass.width;
		image.extent.height = offscreenPass.height;
		image.extent.depth = 1; //must be 1 if the imageType is VK_IMAGE_TYPE_2D
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.format = offscreenPass.depth.format;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VK_CHECK_RESULT(vkCreateImage(device.logical, &image, nullptr, &offscreenPass.depth.image));

		VkMemoryAllocateInfo memAlloc = vk::init::MemoryAllocateInfo();
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device.logical, offscreenPass.depth.image, &memReqs);

		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = device.GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK_RESULT(vkAllocateMemory(device.logical, &memAlloc, nullptr, &offscreenPass.depth.imageMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device.logical, offscreenPass.depth.image, offscreenPass.depth.imageMemory, 0));

		VkImageViewCreateInfo imageView = vk::init::ImageViewCreateInfo();
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageView.format = offscreenPass.depth.format;
		imageView.subresourceRange = {};
		imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imageView.subresourceRange.baseMipLevel = 0;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = offscreenPass.depth.image;
		VK_CHECK_RESULT(vkCreateImageView(device.logical, &imageView, nullptr, &offscreenPass.depth.imageView));

		VkFilter shadowmapFilter = vk::util::FormatIsFilterable(device.physical, offscreenPass.depth.format, VK_IMAGE_TILING_OPTIMAL) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		VkSamplerCreateInfo sampler = vk::init::SamplerCreateInfo();
		sampler.magFilter = shadowmapFilter;
		sampler.minFilter = shadowmapFilter;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.f;
		sampler.maxAnisotropy = 1;
		sampler.minLod = 0.f;
		sampler.maxLod = 1.f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device.logical, &sampler, nullptr, &offscreenPass.depthSampler));


		ShadowMapScene::InitializeOffscreenRenderPass();

		VkFramebufferCreateInfo frameBuffer = vk::init::FramebufferCreateInfo();
		frameBuffer.renderPass = offscreenPass.renderPass;
		frameBuffer.attachmentCount = 1;
		frameBuffer.pAttachments = &offscreenPass.depth.imageView;
		frameBuffer.width = offscreenPass.width;
		frameBuffer.height = offscreenPass.height;
		frameBuffer.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(device.logical, &frameBuffer, nullptr, &offscreenPass.frameBuffer));
	}

	void ShadowMapScene::Render() 
	{
		ContextBase::PrepareFrame();
		UpdateOffscreenUniforms();
		UpdateSceneUniforms();
		RecordCommandBuffers();
		ContextBase::SubmitFrame();
	}

	void ShadowMapScene::ResizeWindow() 
	{
		//won't need to resize the framebuffer for the offscreen pass
		//since it's a fixed size.
		ContextBase::ResizeWindow();
	}
	
}