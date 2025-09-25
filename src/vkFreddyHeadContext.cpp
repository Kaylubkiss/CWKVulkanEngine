#include "vkFreddyHeadContext.h"
#include "ApplicationGlobal.h"
#include "vkInit.h"
#include "vkUtility.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>

namespace vk
{

	FreddyHeadScene::FreddyHeadScene()
	{
		

		//initialized uniforms...
		
		//uniform(s)
		this->sceneUniformBuffer = device.CreateBuffer(sizeof(sceneUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)&sceneUniformData);

		sceneUniformData.light.pos = {};
		sceneUniformData.light.albedo = { 1.0, 1.0, 1.0 };
		sceneUniformData.light.ambient = sceneUniformData.light.albedo * 0.01f;
		sceneUniformData.light.specular = { 0.5f, 0.5f, 0.5f };
		sceneUniformData.light.shininess = 32.f;

		FreddyHeadScene::UpdateUniforms();

		FreddyHeadScene::InitializeDescriptors();
		FreddyHeadScene::InitializePipeline("blinnForward.vert", "blinnForward.frag");

		mInfo.descriptorPool = this->descriptorPool;
		mInfo.descriptorSetLayout = this->descriptorSetLayout;

	}

	FreddyHeadScene::~FreddyHeadScene()
	{
		sceneUniformBuffer.Destroy();

		vkDestroyDescriptorSetLayout(this->device.logical, this->descriptorSetLayout, nullptr);
	}



	void FreddyHeadScene::InitializeScene(ObjectManager& objManager)
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

	void FreddyHeadScene::InitializeDescriptors()
	{

		std::vector<VkDescriptorPoolSize> poolSizes = {
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * 2),
			//we are concerned about the fragment stage, so we double the descriptor count here.
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * 2) //max numbers of frames in flight times two to accomodate the gui.
		};

		VkDescriptorPoolCreateInfo poolInfo = vk::init::DescriptorPoolCreateInfo(poolSizes, 4);
		descriptorPool = vk::init::DescriptorPool(device.logical, poolInfo);


		std::vector<VkDescriptorSetLayoutBinding> dscSetLayoutBindings = {
			//for the scene transform and kight transform
			vk::init::DescriptorLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
			//for the texture sampler
			vk::init::DescriptorLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
		};

		this->descriptorSetLayout = vk::init::DescriptorSetLayout(device.logical, dscSetLayoutBindings.data(), (uint32_t)dscSetLayoutBindings.size());

		sceneDescriptorSet = vk::init::DescriptorSet(device.logical, descriptorPool, descriptorSetLayout);


		VkDescriptorImageInfo defaultTextureDescriptor = {};
		defaultTextureDescriptor.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		defaultTextureDescriptor.imageView = defaultTexture.mTextureImageView;
		defaultTextureDescriptor.sampler = defaultTexture.mTextureSampler;

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			//uniform transforms
			vk::init::WriteDescriptorSet(sceneDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &sceneUniformBuffer.descriptor),
			vk::init::WriteDescriptorSet(sceneDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &defaultTextureDescriptor)
		};

		vkUpdateDescriptorSets(device.logical, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

		//TODO: janky mInfo stuff for texturemanager......
		writeDescriptorSets = 
		{
			vk::init::WriteDescriptorSet(sceneDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &sceneUniformBuffer.descriptor)
		};

		mInfo.sceneWriteDescriptorSets = writeDescriptorSets;
	}

	void FreddyHeadScene::RecordCommandBuffers(vk::ObjectManager& objManager)
	{
		for (size_t i = 0; i < this->commandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			//everything else is default...

			//resetting command buffer should be implicit with reset flag set on this.
			VK_CHECK_RESULT(vkBeginCommandBuffer(this->commandBuffers[i], &cmdBufferBeginInfo))

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = this->mPipeline.RenderPass();
			renderPassInfo.framebuffer = this->swapChain.frameBuffers[i];
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = this->currentExtent;

			VkClearValue clearColors[2] = {};

			uLightObject& sceneLight = sceneUniformData.light;
			clearColors[0].color = { sceneLight.ambient.x, sceneLight.ambient.y, sceneLight.ambient.z, 1.f };
			clearColors[1].depthStencil = { 1.f, 0 };

			renderPassInfo.clearValueCount = 2;
			renderPassInfo.pClearValues = clearColors;

			vkCmdBeginRenderPass(this->commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			//this could be overwritten with the descriptor set of an object with a texture.
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.layout, 0, 1, &sceneDescriptorSet, 0, nullptr);

			vkCmdBindPipeline(this->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->mPipeline.Handle());

			vkCmdSetViewport(this->commandBuffers[i], 0, 1, &window.viewport);
			vkCmdSetScissor(this->commandBuffers[i], 0, 1, &window.scissor);

			objManager.DrawObjects(this->commandBuffers[i], this->mPipeline.Layout());

			vkCmdEndRenderPass(this->commandBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(this->commandBuffers[i]));
		}

	}

	void FreddyHeadScene::UpdateUniforms() 
	{
		Camera& appCamera = _Application->GetCamera();
		//uniform transform for objects of default pipeline.
		this->sceneUniformData.transform = {
			appCamera.LookAt(), //view
			glm::perspective(glm::radians(45.f), (float)window.viewport.width / window.viewport.height, 0.1f, 1000.f) //proj
		};

		this->sceneUniformData.transform.proj[1][1] *= -1.f;

		this->sceneUniformData.camPos = appCamera.Position();

		memcpy(this->sceneUniformBuffer.mappedMemory, &this->sceneUniformData, sizeof(sceneUniformData));
	}

	void FreddyHeadScene::Render() 
	{
		UpdateUniforms();
		ContextBase::Render();
	}

	void FreddyHeadScene::InitializePipeline(std::string vsFile, std::string fsFile)
	{
		ShaderModuleInfo vertShaderInfo(this->device.logical, vsFile, VK_SHADER_STAGE_VERTEX_BIT);
		ShaderModuleInfo fragShaderInfo(this->device.logical, fsFile, VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);

		//this is for an object's model transformation.
		std::vector<VkPushConstantRange> pushConstants = {
			vk::init::PushConstantRange(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
		};

		VkPipelineLayout pipelineLayout = vk::init::CreatePipelineLayout(this->device.logical, descriptorSetLayout, pushConstants);

		this->mPipeline.
			AddModule(vertShaderInfo).
			AddModule(fragShaderInfo).
			AddPipelineLayout(pipelineLayout).
			Finalize(this->device.logical, this->device.physical,
				window, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
			);


	}

}