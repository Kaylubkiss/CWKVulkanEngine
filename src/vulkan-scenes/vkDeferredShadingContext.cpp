#include "vkDeferredShadingContext.h"

//TODO: be able to specify the objects you want in the scene at compile time.
//TODO: remove camel case -- looks ugly and a bit unreadable. There is some inconsistency here in this file with that. 
#define OBJECT_COUNT (10 + 1) //max 10 objects in the scene, +1 for blank texture
inline VkDeviceSize AlignedSize(VkDeviceSize size, VkDeviceSize alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

namespace vk
{

	DeferredContext::DeferredContext()
	{

		framebuffers.deMRT.width = 2048;
		framebuffers.deMRT.height = 2048;

		framebuffers.deShadow.width = 2048;
		framebuffers.deShadow.height = 2048;

		DeferredContext::InitializeUniforms();
		DeferredContext::InitializeDeferredFramebuffer();
		DeferredContext::InitializeDeferredShadowFramebuffer();
		DeferredContext::InitializeDescriptors();
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

		for (auto& pl : pipelineLayouts)
		{
			vkDestroyPipelineLayout(device.logical, pl, nullptr);
		}

		for (auto& uniformDescriptor : uniformBindingDescriptors) 
		{
			uniformDescriptor.Destroy();
		}

		textureBindingDescriptor.Destroy();
		compositionImageBindingDescriptor.Destroy();

		framebuffers.deMRT.Destroy();
		framebuffers.deShadow.Destroy();
	}

	void DeferredContext::InitializeScene( ObjectManager* objManager )
	{
		ObjectCreateInfo objectCI = {};
		
		//object 1 - freddy
		objectCI.objName = "freddy.obj";
		objectCI.textureFileName = "myface.JPG";
		objectCI.modelTransform = glm::translate(glm::mat4(1.f), sceneSettings.freddyPosition) * 
			glm::scale(glm::mat4(1.f), glm::vec3(3.f));
		objectCI.devicePtr = &this->device;

		objManager->LoadObject(objectCI);

		//object 2 - cube
		objectCI = {};

		PhysicsComponent physicsComponent;
		physicsComponent.bodyType = BodyType::DYNAMIC;
		physicsComponent.colliderType = PhysicsComponent::ColliderType::CUBE;
		
		objectCI.objName = "cube.obj";
		//NOTE: cube.obj doesn't have UVs.
		objectCI.textureFileName = "myface.JPG";
		objectCI.physicsComponent = physicsComponent;
		objectCI.hasPhysicsComponent = true;
		objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(sceneSettings.cubePosition));
		objectCI.devicePtr = &this->device;

		objManager->LoadObject(objectCI);

		//object 3 - base
		objectCI = {};
		
		physicsComponent.bodyType = reactphysics3d::BodyType::STATIC;

		objectCI.objName = "base.obj";
		objectCI.textureFileName = "wood-floor.png";
		objectCI.physicsComponent = physicsComponent;
		objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0, -5.f, 0)) *
			glm::scale(glm::mat4(1.f), glm::vec3(30.f));
		objectCI.hasPhysicsComponent = true;
		objectCI.devicePtr = &this->device;

		objManager->LoadObject(objectCI);

		objectCI = {};

		objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0, 5.f, 0));
		objectCI.objName = "AnimatedCube.gltf";
		objectCI.devicePtr = &this->device;

		objManager->LoadObject(objectCI);

	}

	void DeferredContext::ResizeWindowDerived()
	{
		//MRT resizing...

		InitializeDeferredFramebuffer();

		//shadow resizing...
		InitializeDeferredShadowFramebuffer();

		for (int frame = 0; frame < gMaxFramesInFlight; ++frame) 
		{
			char* image_descriptor_ptr = (char*)compositionImageBindingDescriptor.buffers[frame].GetMappedMemory();
			for (int i = 0; i < RT_COUNT; ++i)
			{
				//info
				VkDescriptorImageInfo rt_descriptor_image_info;
				auto& fb_attachment = framebuffers.deMRT.attachments[i];
				rt_descriptor_image_info.imageLayout = fb_attachment.layout;
				rt_descriptor_image_info.imageView = fb_attachment.imageView;
				rt_descriptor_image_info.sampler = framebuffers.deMRT.sampler;

				//get info
				VkDescriptorGetInfoEXT rt_descriptor_get_infos = {};
				rt_descriptor_get_infos = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
				rt_descriptor_get_infos.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				rt_descriptor_get_infos.data.pCombinedImageSampler = &rt_descriptor_image_info;

				g_vkGetDescriptorEXT(device.logical, &rt_descriptor_get_infos,
					device.DescriptorBufferProperties().combinedImageSamplerDescriptorSize,
					image_descriptor_ptr + compositionImageBindingDescriptor.binding_offsets[i]);
			}

			//info - shadow target
			VkDescriptorImageInfo rt_descriptor_image_info;
			auto& fb_attachment = framebuffers.deShadow.attachments.front();
			rt_descriptor_image_info.imageLayout = fb_attachment.layout;
			rt_descriptor_image_info.imageView = fb_attachment.imageView;
			rt_descriptor_image_info.sampler = framebuffers.deShadow.sampler;

			//get info - shadow
			VkDescriptorGetInfoEXT rt_descriptor_get_infos = {};
			rt_descriptor_get_infos = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
			rt_descriptor_get_infos.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			rt_descriptor_get_infos.data.pCombinedImageSampler = &rt_descriptor_image_info;

			g_vkGetDescriptorEXT(device.logical, &rt_descriptor_get_infos,
				device.DescriptorBufferProperties().combinedImageSamplerDescriptorSize,
				image_descriptor_ptr + compositionImageBindingDescriptor.binding_offsets.back());
		}
	}

	void DeferredContext::FillOutGraphicsContextInfo()
	{
		mInfo.contextTextureDescriptorPtr = &textureBindingDescriptor;
	}

	void DeferredContext::InitializeUniforms()
	{
		for (size_t i = 0; i < uniformBuffers.size(); ++i) {
			//////////////////////////////////
			//#1 - deferred MRT
			uniformBuffers[i].mrt = device.CreateBuffer(sizeof(uniformDataMRT),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataMRT));
			uniformBuffers[i].mrt.Map(); //persistent data

			//initializing light positions
			uniformDataLightPass.lights[0].pos = { 3, 27, -14 };
			uniformDataLightPass.lights[1].pos = { 33, 33, 30 };

			//////////////////////////////////
			//#2 - deferred shadow
			glm::mat4 perspective = glm::perspective(glm::radians(lightFOV), 1.f, zNear, zFar);
			uniformDataDeferredShadow.viewMatrices[0] = perspective * glm::lookAt(uniformDataLightPass.lights[0].pos,
				sceneSettings.freddyPosition, glm::vec3(0, 1, 0));
			uniformDataDeferredShadow.viewMatrices[1] = perspective * glm::lookAt(uniformDataLightPass.lights[1].pos,
				sceneSettings.cubePosition, glm::vec3(0, 1, 0));

			uniformBuffers[i].shadow = device.CreateBuffer(sizeof(uniformDataDeferredShadow),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataDeferredShadow));
			uniformBuffers[i].shadow.Map(); //persistent data

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

			uniformBuffers[i].composition = device.CreateBuffer(sizeof(uniformDataLightPass),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				(void*)(&uniformDataLightPass));
			uniformBuffers[i].composition.Map();
		}
	}

	inline void GetDescriptorLayoutSize(const vk::Device* device, VkDescriptorSetLayout layout, VkDeviceSize* size)
	{
		assert(size);
		g_vkGetDescriptorSetLayoutSizeEXT(device->logical, layout, size);
		*size = AlignedSize(*size, device->DescriptorBufferProperties().descriptorBufferOffsetAlignment);
	}

	inline void GetDescriptorLayoutBindingOffsets( const vk::Device* device, VkDescriptorSetLayout layout,
		VkDeviceSize offsets[], uint32_t binding_count = 1 )
	{
		//get the offsets of each descriptor binding in the layout 
		for (int i = 0; i < binding_count; ++i)
		{
			g_vkGetDescriptorSetLayoutBindingOffsetEXT(device->logical, layout, i, &offsets[i]);
		}
	}

	void DeferredContext::InitializeDescriptorLayouts()
	{
		std::array<VkDescriptorSetLayoutBinding, 4> setLayoutBindings = {}; //up to 4 bindings -- particularly for the composition pass

		VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo =
			vk::init::DescriptorSetLayoutCreateInfo(setLayoutBindings.data(), 1);
		setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

		//MRT PASS DESCRIPTORS
		{

			//per-frame scene transform
			setLayoutBindings[0] =
				vk::init::DescriptorLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					VK_SHADER_STAGE_VERTEX_BIT);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(this->device.logical, &setLayoutCreateInfo,
				nullptr, &uniformBindingDescriptors[dePipeline::MRT].layout));

			//per-model image samplers
			setLayoutBindings[0] =
				vk::init::DescriptorLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(this->device.logical, &setLayoutCreateInfo,
				nullptr, &textureBindingDescriptor.layout));

			std::vector<VkPushConstantRange> pushConstantRanges = {
				vk::init::PushConstantRange(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
			};

			std::array<VkDescriptorSetLayout, 2> mrt_layouts = {
				//set 0: per-frame scene transform, set 1: per-model image sampler (TODO: use set 1 as model transform)
				uniformBindingDescriptors[dePipeline::MRT].layout, textureBindingDescriptor.layout,
			};

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
			pipelineLayoutCreateInfo.pSetLayouts = mrt_layouts.data();
			pipelineLayoutCreateInfo.setLayoutCount = mrt_layouts.size();
			pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
			pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
			VK_CHECK_RESULT(vkCreatePipelineLayout(device.logical, &pipelineLayoutCreateInfo,
				nullptr, &pipelineLayouts[dePipeline::MRT]));

		}

		//COMPOSITION PASS DESCRIPTORS
		{

			//set 1: per-frame light data
			setLayoutBindings[0] =
				vk::init::DescriptorLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					VK_SHADER_STAGE_FRAGMENT_BIT);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(this->device.logical, &setLayoutCreateInfo,
				nullptr, &uniformBindingDescriptors[dePipeline::COMPOSITION].layout));

			//set 0: per-frame image resources
			setLayoutCreateInfo = vk::init::DescriptorSetLayoutCreateInfo(setLayoutBindings.data(),
				setLayoutBindings.size());
			setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			for (uint32_t i = 0; i < setLayoutBindings.size(); ++i)
			{
				setLayoutBindings[i] = vk::init::DescriptorLayoutBinding(i, 1,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
			}
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(this->device.logical, &setLayoutCreateInfo,
				nullptr, &compositionImageBindingDescriptor.layout));

			//COMPOSITION PASS g buffer image descriptors
			compositionImageBindingDescriptor.c_device = device.logical;
			GetDescriptorLayoutSize(&device, compositionImageBindingDescriptor.layout,
				&compositionImageBindingDescriptor.size);

			//may be storing the same offset across these binding offsets since
			//it's the same descriptor type (COMBINED_IMAGE_SAMPLER)

			//+1 for the shadow descriptor

			compositionImageBindingDescriptor.binding_offsets.resize(RT_COUNT + 1);
			GetDescriptorLayoutBindingOffsets(&device, compositionImageBindingDescriptor.layout,
				compositionImageBindingDescriptor.binding_offsets.data(), RT_COUNT + 1);

			//order of layouts need to be in order of they appear in shader(s)
			std::array<VkDescriptorSetLayout, 2> composition_layouts = {
				compositionImageBindingDescriptor.layout, uniformBindingDescriptors[dePipeline::COMPOSITION].layout
			};
			
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
			pipelineLayoutCreateInfo.pSetLayouts = composition_layouts.data();
			pipelineLayoutCreateInfo.setLayoutCount = composition_layouts.size();
			VK_CHECK_RESULT(vkCreatePipelineLayout(device.logical, &pipelineLayoutCreateInfo,
				nullptr, &pipelineLayouts[dePipeline::COMPOSITION]));
		}

		//SHADOW MAP DESCRIPTORS
		{
			VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo =
				vk::init::DescriptorSetLayoutCreateInfo(setLayoutBindings.data(), 1);
			setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			setLayoutBindings[0] =
				vk::init::DescriptorLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					VK_SHADER_STAGE_GEOMETRY_BIT);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(this->device.logical, &setLayoutCreateInfo,
				nullptr, &uniformBindingDescriptors[dePipeline::SHADOW].layout));

			//set 0: shadow UBO - per frame
			std::array<VkDescriptorSetLayout, 1> shadow_layouts = {
				uniformBindingDescriptors[dePipeline::SHADOW].layout
			};


			//per-model transform
			std::vector<VkPushConstantRange> pushConstantRanges = {
				vk::init::PushConstantRange(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
			};

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::init::PipelineLayoutCreateInfo();
			pipelineLayoutCreateInfo.pSetLayouts = shadow_layouts.data();
			pipelineLayoutCreateInfo.setLayoutCount = shadow_layouts.size();
			pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
			pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
			VK_CHECK_RESULT(vkCreatePipelineLayout(device.logical, &pipelineLayoutCreateInfo,
				nullptr, &pipelineLayouts[dePipeline::SHADOW]));

		}

		//UNIFORM DATA  descriptors (all passes)
		for (auto& uniformDescriptor : uniformBindingDescriptors)
		{
			uniformDescriptor.c_device = device.logical;
			GetDescriptorLayoutSize(&device, uniformDescriptor.layout, &uniformDescriptor.size);
			GetDescriptorLayoutBindingOffsets(&device, uniformDescriptor.layout,
				uniformDescriptor.binding_offsets.data());
		}
	
		//TEXTURE IMAGE descriptor (static)
		textureBindingDescriptor.c_device = device.logical;
		GetDescriptorLayoutSize(&device, textureBindingDescriptor.layout, &textureBindingDescriptor.size);
		GetDescriptorLayoutBindingOffsets(&device, textureBindingDescriptor.layout,
			textureBindingDescriptor.binding_offsets.data());

	}

	inline void GetUniformDescriptor( vk::Buffer& descriptorBuffer, const vk::Buffer& dataBuffer,
		const vk::Device& device )
	{
		char* descriptor_ptr = (char*)descriptorBuffer.GetMappedMemory();
		VkDescriptorAddressInfoEXT addrInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
		addrInfo.address = dataBuffer.GetDeviceAddress();
		addrInfo.range = dataBuffer.GetSize();
		addrInfo.format = VK_FORMAT_UNDEFINED;

		VkDescriptorGetInfoEXT bufferDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		bufferDescriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bufferDescriptorInfo.data.pUniformBuffer = &addrInfo;

		g_vkGetDescriptorEXT(device.logical, &bufferDescriptorInfo,
			device.DescriptorBufferProperties().uniformBufferDescriptorSize,
			descriptor_ptr);
	}

	void DeferredContext::InitializeDescriptorBuffers() 
	{
		//TODO: need to remove and properly initialize the descriptor's buffer here
		//1 for scene uniform in MRT pass, 1 for composition pass, 1 for scene uniform shadow pass
		for (int frame = 0; frame < gMaxFramesInFlight; ++frame) 
		{
			//MRT pass UBO (global transforms)
			uniformBindingDescriptors[dePipeline::MRT].buffers[frame] =
				vk::Buffer(&device,
					VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | 
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
					VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					uniformBindingDescriptors[dePipeline::MRT].size);

			uniformBindingDescriptors[dePipeline::MRT].buffers[frame].Map(); //persistent

		
			GetUniformDescriptor(
				uniformBindingDescriptors[dePipeline::MRT].buffers[frame],
				uniformBuffers[frame].mrt, device
			);

			//shadow pass UBO (light POV)
			uniformBindingDescriptors[dePipeline::SHADOW].buffers[frame] =
				vk::Buffer(&device,
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | 
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniformBindingDescriptors[dePipeline::SHADOW].size);

			uniformBindingDescriptors[dePipeline::SHADOW].buffers[frame].Map(); //persistent
	
			GetUniformDescriptor(
				uniformBindingDescriptors[dePipeline::SHADOW].buffers[frame],
				uniformBuffers[frame].shadow, device
			);

			//composition UBO (light data)
			uniformBindingDescriptors[dePipeline::COMPOSITION].buffers[frame] =
				vk::Buffer(&device,
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | 
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniformBindingDescriptors[dePipeline::COMPOSITION].size);

			uniformBindingDescriptors[dePipeline::COMPOSITION].buffers[frame].Map(); //persistent

			GetUniformDescriptor(uniformBindingDescriptors[dePipeline::COMPOSITION].buffers[frame],
				uniformBuffers[frame].composition, device);

			//Composition Image Samplers 
			compositionImageBindingDescriptor.buffers[frame] = vk::Buffer(&device, 
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | 
				VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				compositionImageBindingDescriptor.size);


			compositionImageBindingDescriptor.buffers[frame].Map();
			char* image_descriptor_ptr = (char*)compositionImageBindingDescriptor.buffers[frame].GetMappedMemory();
			for (int i = 0; i < RT_COUNT; ++i) 
			{
				//info
				VkDescriptorImageInfo rt_descriptor_image_info;
				auto& fb_attachment = framebuffers.deMRT.attachments[i];
				rt_descriptor_image_info.imageLayout = fb_attachment.layout;
				rt_descriptor_image_info.imageView = fb_attachment.imageView;
				rt_descriptor_image_info.sampler = framebuffers.deMRT.sampler;

				//get info
				VkDescriptorGetInfoEXT rt_descriptor_get_infos = {};
				rt_descriptor_get_infos = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
				rt_descriptor_get_infos.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				rt_descriptor_get_infos.data.pCombinedImageSampler = &rt_descriptor_image_info;

				g_vkGetDescriptorEXT(device.logical, &rt_descriptor_get_infos,
					device.DescriptorBufferProperties().combinedImageSamplerDescriptorSize,
					image_descriptor_ptr + compositionImageBindingDescriptor.binding_offsets[i]);
			}

			//info - shadow target
			VkDescriptorImageInfo rt_descriptor_image_info;
			auto& fb_attachment = framebuffers.deShadow.attachments.front();
			rt_descriptor_image_info.imageLayout = fb_attachment.layout;
			rt_descriptor_image_info.imageView = fb_attachment.imageView;
			rt_descriptor_image_info.sampler = framebuffers.deShadow.sampler;

			//get info - shadow
			VkDescriptorGetInfoEXT rt_descriptor_get_infos = {};
			rt_descriptor_get_infos = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
			rt_descriptor_get_infos.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			rt_descriptor_get_infos.data.pCombinedImageSampler = &rt_descriptor_image_info;

			g_vkGetDescriptorEXT(device.logical, &rt_descriptor_get_infos,
				device.DescriptorBufferProperties().combinedImageSamplerDescriptorSize,
				image_descriptor_ptr + compositionImageBindingDescriptor.binding_offsets.back());
		}

		//texture sampler buffer - static buffer.
		textureBindingDescriptor.buffers.front() = vk::Buffer(&device,
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | 
			VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | 
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			OBJECT_COUNT * textureBindingDescriptor.size);

		//fill in the default texture first, then try to figure out texture manager.
		textureBindingDescriptor.buffers.front().Map();
	}

	void DeferredContext::UpdateScreenUniforms()
	{
		//transform(s)
		uniformDataMRT.uTransform =
		{
			mCamera.LookAt(),
			glm::perspective(glm::radians(FOV),
				(float)window.viewport.width / window.viewport.height, 0.1f, 1000.f)
		};

		uniformDataMRT.uTransform.proj[1][1] *= -1;

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
		uniformDataLightPass.viewPosition = mCamera.Position();
		uniformDataLightPass.lights[0].viewMatrix = uniformDataDeferredShadow.viewMatrices[0];
		uniformDataLightPass.lights[1].viewMatrix = uniformDataDeferredShadow.viewMatrices[1];
		memcpy(uniformBuffers[currentFrame].composition.GetMappedMemory(), (void*)(&uniformDataLightPass),
			sizeof(uniformDataLightPass));
	}

	void DeferredContext::DrawObjectsWithTexture(
		VkCommandBuffer cmdBuffer, 
		VkPipelineLayout pipelineLayout, 
		ObjectManager& objManager) 
	{
		vk::DrawInfo drawInfo = {};
		drawInfo.imageBufferIndex = 1;
		drawInfo.cmdBuffer = cmdBuffer;
		drawInfo.setCount = 1;
		drawInfo.firstSet = 0;
		drawInfo.pipelineLayout = pipelineLayout;
		drawInfo.textureBindingSize = textureBindingDescriptor.size;

		objManager.DrawObjects(drawInfo);
	}

	void DeferredContext::InitializeDeferredFramebuffer()
	{
		framebuffers.deMRT.Destroy();
		framebuffers.deMRT.Init(&this->device);

		VkFramebufferCreateInfo framebuffer = vk::init::FramebufferCreateInfo();
		framebuffer.width = framebuffers.deMRT.width;
		framebuffer.height = framebuffers.deMRT.height;
		framebuffer.layers = 1;
		
		vk::FramebufferAttachmentCreateInfo attachmentCI = {};

		//position attachment
		attachmentCI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		attachmentCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		attachmentCI.width = framebuffer.width;
		attachmentCI.height = framebuffer.height;
		framebuffers.deMRT.AddAttachment(attachmentCI);
		
		//normal attachment
		framebuffers.deMRT.AddAttachment(attachmentCI);

		//albedo attachment
		attachmentCI.format = VK_FORMAT_R8G8B8A8_UNORM;
		framebuffers.deMRT.AddAttachment(attachmentCI);

		//depth attachment
		attachmentCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
		attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		framebuffers.deMRT.AddAttachment(attachmentCI);

		framebuffers.deMRT.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		framebuffers.deMRT.CreateRenderPass();

		framebuffers.deMRT.CreateFramebuffer();

	}

	void DeferredContext::InitializeDeferredShadowFramebuffer() 
	{
		framebuffers.deShadow.Destroy();
		framebuffers.deShadow.Init(&this->device);

		vk::FramebufferAttachmentCreateInfo attachmentCI = {};

		attachmentCI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		attachmentCI.width = framebuffers.deShadow.width;
		attachmentCI.height = framebuffers.deShadow.height;
		attachmentCI.layerCount = LIGHT_COUNT;
		attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		framebuffers.deShadow.AddAttachment(attachmentCI);
		framebuffers.deShadow.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		framebuffers.deShadow.CreateRenderPass();
		framebuffers.deShadow.CreateFramebuffer();
	}

	void DeferredContext::InitializeDescriptors() 
	{

		//MOVING TO DESCRIPTOR BUFFERS; 
		// no vkCreateDescriptorSet or vkWriteDescriptorSet() or vkCreateDescriptorPool()
		InitializeDescriptorLayouts();
		InitializeDescriptorBuffers();
	}
	
	void DeferredContext::InitializePipeline(std::string vsFile, std::string fsFile)
	{
		(void)vsFile;
		(void)fsFile;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = 
			vk::init::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0,
				VK_FALSE);
		
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = 
			vk::init::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
				VK_FRONT_FACE_COUNTER_CLOCKWISE);
		
		VkPipelineColorBlendAttachmentState blendAttachmentState = 
			vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
		
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = 
			vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = 
			vk::init::PipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE,
				VK_COMPARE_OP_LESS_OR_EQUAL);
		
		VkPipelineViewportStateCreateInfo viewportStateCI = 
			vk::init::PipelineViewportStateCreateInfo(1, 1);
		
		VkPipelineMultisampleStateCreateInfo multiplesampleStateCI = 
			vk::init::PipelineMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicStateCI = 
			vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vk::init::PipelineCreateInfo(pipelineLayouts[dePipeline::COMPOSITION],
			renderPass, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT);
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

		vk::ShaderModuleInfo vertShaderInfo = {};
		vk::ShaderModuleInfo fragShaderInfo = {};

		/////////////////////////////////////////////////////////////
		//pipeline #1: composition stage of deferred shading
		{
			vertShaderInfo = vk::ShaderModuleInfo(device.logical,
				"deferredLightPass.vert", VK_SHADER_STAGE_VERTEX_BIT);
			fragShaderInfo = vk::ShaderModuleInfo(device.logical,
				"deferredLightPass.frag", VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);

			shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
			shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.mHandle, fragShaderInfo.mFlags);

			rasterizationStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;

			pipelineManager.AddModule(dePipeline::COMPOSITION, vertShaderInfo);
			pipelineManager.AddModule(dePipeline::COMPOSITION, fragShaderInfo);

			VkPipeline lightPassPipeline = VK_NULL_HANDLE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI,
				nullptr, &lightPassPipeline));

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

					VkPipeline pipeline = pipelineManager.Get(dePipeline::COMPOSITION);

					if (pipeline != VK_NULL_HANDLE)
					{
						vkDestroyPipeline(device.logical, pipeline, nullptr);
						pipeline = VK_NULL_HANDLE;
					}

					VkPipelineColorBlendAttachmentState blendAttachmentState =
						vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
					VkPipelineColorBlendStateCreateInfo colorBlendStateCI =
						vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

					std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
					VkPipelineDynamicStateCreateInfo dynamicStateCI =
						vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

					std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

					VkGraphicsPipelineCreateInfo pipelineCI = 
						vk::init::PipelineCreateInfo(
							pipelineLayouts[dePipeline::COMPOSITION], renderPass, 
							VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT);

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

					const std::vector<ShaderModuleInfo>& shaders =
						pipelineManager.GetPipelineShaders(dePipeline::COMPOSITION);
					shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(shaders[0].mHandle, shaders[0].mFlags);
					shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(shaders[1].mHandle, shaders[1].mFlags);

					VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1,
						&pipelineCI, nullptr, &pipeline));

					pipelineManager.AddPipeline(dePipeline::COMPOSITION, pipeline);
				};


			pipelineManager.AddPipeline(dePipeline::COMPOSITION, lightPassPipeline,
				std::move(lightPassCreationFunction));
		}

	
		/////////////////////////////////////////////////////////////
		//pipeline #2: MRT stage of deferred shading -- outputting to color/textures
		{
			vertShaderInfo = ShaderModuleInfo(device.logical,
				"deferredMRT.vert", VK_SHADER_STAGE_VERTEX_BIT);
			fragShaderInfo = ShaderModuleInfo(device.logical,
				"deferredMRT.frag", VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);


			shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.mHandle, vertShaderInfo.mFlags);
			shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.mHandle, fragShaderInfo.mFlags);


			rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;

			pipelineCI.layout = pipelineLayouts[dePipeline::MRT];
			pipelineCI.renderPass = framebuffers.deMRT.renderPass;

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
			auto vertexInputAttributeDescriptions =
				Vertex::InputAttributeDescriptions();

			VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
			vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
			vertexInputStateCI.vertexBindingDescriptionCount = 1;
			vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
			vertexInputStateCI.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();

			pipelineCI.pVertexInputState = &vertexInputStateCI;


			pipelineManager.AddModule(dePipeline::MRT, vertShaderInfo);
			pipelineManager.AddModule(dePipeline::MRT, fragShaderInfo);

			VkPipeline mrtPipeline = VK_NULL_HANDLE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1,
				&pipelineCI, nullptr, &mrtPipeline));

			//for hot reloading - MRT pass
			std::function<void()> MRTPassCreationFunction =
				[this,
				inputAssemblyStateCI,
				rasterizationStateCI,
				depthStencilStateCI,
				multiplesampleStateCI,
				viewportStateCI]
				{

					VkPipeline pipeline = pipelineManager.Get(dePipeline::MRT);

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
					VkPipelineDynamicStateCreateInfo dynamicStateCI =
						vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

					VkVertexInputBindingDescription vertexBindingDescription =
						vk::init::VertexInputBindingDescription();
					auto vertexInputAttributeDescriptions =
						Vertex::InputAttributeDescriptions();

					VkPipelineVertexInputStateCreateInfo vertexInputStateCI =
						vk::init::PipelineVertexInputStateCreateInfo();
					vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
					vertexInputStateCI.vertexBindingDescriptionCount = 1;
					vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
					vertexInputStateCI.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();

					std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

					VkGraphicsPipelineCreateInfo pipelineCI = 
						vk::init::PipelineCreateInfo(pipelineLayouts[dePipeline::MRT], framebuffers.deMRT.renderPass,
							VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT);

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

					const std::vector<ShaderModuleInfo>& shaders = pipelineManager.GetPipelineShaders(dePipeline::MRT);
					shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(shaders[0].mHandle, shaders[0].mFlags);
					shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(shaders[1].mHandle, shaders[1].mFlags);

					VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1,
						&pipelineCI, nullptr, &pipeline));

					pipelineManager.AddPipeline(dePipeline::MRT, pipeline);
				};


			pipelineManager.AddPipeline(dePipeline::MRT, mrtPipeline, std::move(MRTPassCreationFunction));
		}


		/////////////////////////////////////////////////////////////
		//pipeline #3: deferred shadow mapping
		{
			vertShaderInfo = ShaderModuleInfo(device.logical, "deferredShadow.vert",
				VK_SHADER_STAGE_VERTEX_BIT);
			ShaderModuleInfo geoShaderInfo(device.logical, "deferredShadow.geom",
				VK_SHADER_STAGE_GEOMETRY_BIT, shaderc_geometry_shader);

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

			pipelineCI.renderPass = framebuffers.deShadow.renderPass;
			pipelineCI.layout = pipelineLayouts[dePipeline::SHADOW];

			//shadow pass only consumes the position of vertices
			VkVertexInputBindingDescription vertexBindingDescription = vk::init::VertexInputBindingDescription();
			VkVertexInputAttributeDescription vertexPosAttributeDescripton = {};
			vertexPosAttributeDescripton.format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexPosAttributeDescripton.location = 0;
			vertexPosAttributeDescripton.binding = 0;
			vertexPosAttributeDescripton.offset = offsetof(struct Vertex, pos);

			VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
			vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
			vertexInputStateCI.vertexBindingDescriptionCount = 1;
			vertexInputStateCI.pVertexAttributeDescriptions = &vertexPosAttributeDescripton;
			vertexInputStateCI.vertexAttributeDescriptionCount = 1;

			pipelineCI.pVertexInputState = &vertexInputStateCI;

			VkPipeline shadowPipeline = VK_NULL_HANDLE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCI,
				nullptr, &shadowPipeline));

			pipelineManager.AddModule(dePipeline::SHADOW, vertShaderInfo);
			pipelineManager.AddModule(dePipeline::SHADOW, geoShaderInfo);
			pipelineManager.AddPipeline(dePipeline::SHADOW, shadowPipeline, nullptr);
		}
	}

	void DeferredContext::RecordCommandBuffers()
	{
		ObjectManager& objManager = _Application->GetObjectManager();

		VkCommandBuffer cmdBuffer = commandBuffers[currentFrame];
		VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();

		//clear value count corresponds to the number of attachments.
		VkClearValue clearValues[4]; //position, normal, albedo, depth;

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));

		//Shadow depth writes
		{
			clearValues[0].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBI = vk::init::RenderPassBeginInfo();
			renderPassBI.clearValueCount = 1;
			renderPassBI.pClearValues = clearValues;
			renderPassBI.renderArea.extent = { (uint32_t)framebuffers.deShadow.width,
				(uint32_t)framebuffers.deShadow.height };
			renderPassBI.renderPass = framebuffers.deShadow.renderPass;
			renderPassBI.framebuffer = framebuffers.deShadow.handle;

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineManager.Get(dePipeline::SHADOW));
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport shadowViewport = vk::init::Viewport(framebuffers.deShadow.width, framebuffers.deShadow.height);
			vkCmdSetViewport(cmdBuffer, 0, 1, &shadowViewport);

			VkRect2D shadowScissor = vk::init::Rect2D(framebuffers.deShadow.width, framebuffers.deShadow.height);
			vkCmdSetScissor(cmdBuffer, 0, 1, &shadowScissor);

			vkCmdSetDepthBias(cmdBuffer, depthBiasConstant, 0.f, depthBiasSlope);

			// Binding 0 = uniform buffer
			std::array<VkDescriptorBufferBindingInfoEXT, 1> descriptor_buffer_binding_info = {};
			descriptor_buffer_binding_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[0].address = 
				uniformBindingDescriptors[dePipeline::SHADOW].buffers[currentFrame].GetDeviceAddress();
			descriptor_buffer_binding_info[0].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, descriptor_buffer_binding_info.size(),
				descriptor_buffer_binding_info.data());

			uint32_t buffer_index_ubo = 0;
			VkDeviceSize buffer_offset = 0;

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::SHADOW], 0, 1, &buffer_index_ubo, &buffer_offset);

			vk::DrawInfo drawInfo = {};
			drawInfo.cmdBuffer = cmdBuffer;
			drawInfo.pipelineLayout = pipelineLayouts[dePipeline::SHADOW];
			objManager.DrawObjects(drawInfo);

			vkCmdEndRenderPass(cmdBuffer);
		}

		//MRT rendering.
		{
			clearValues[0].color =
			{ 0,0,0,0 };
			clearValues[1].color = clearValues[0].color;
			clearValues[2].color = clearValues[0].color;
			clearValues[3].depthStencil = { 1.f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = 4;
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.renderArea.extent = { (uint32_t)framebuffers.deMRT.width,
				(uint32_t)framebuffers.deMRT.height };
			renderPassBeginInfo.renderPass = framebuffers.deMRT.renderPass;
			renderPassBeginInfo.framebuffer = framebuffers.deMRT.handle;

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.Get(dePipeline::MRT));
			
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport mrtViewport = vk::init::Viewport(framebuffers.deMRT.width, framebuffers.deMRT.height);
			vkCmdSetViewport(cmdBuffer, 0, 1, &mrtViewport);

			VkRect2D mrtScissor = vk::init::Rect2D(framebuffers.deMRT.width, framebuffers.deMRT.height);
			vkCmdSetScissor(cmdBuffer, 0, 1, &mrtScissor);

			// Binding 0 = uniform buffer
			std::array<VkDescriptorBufferBindingInfoEXT, 2> descriptor_buffer_binding_info = {};
			descriptor_buffer_binding_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[0].address = 
				uniformBindingDescriptors[dePipeline::MRT].buffers[currentFrame].GetDeviceAddress();
			descriptor_buffer_binding_info[0].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
			// Binding 1 = Image
			descriptor_buffer_binding_info[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[1].address = 
				textureBindingDescriptor.buffers.front().GetDeviceAddress(); //only need one texture buffer.
			descriptor_buffer_binding_info[1].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, descriptor_buffer_binding_info.size(),
				descriptor_buffer_binding_info.data());

			uint32_t buffer_index_ubo = 0;
			VkDeviceSize buffer_offset = 0;
			
			//global transform
			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::MRT], 0, 1, &buffer_index_ubo, &buffer_offset);

			vk::DrawInfo drawInfo = {};
			drawInfo.cmdBuffer = cmdBuffer;
			drawInfo.imageBufferIndex = 1;
			drawInfo.firstSet = 1;
			drawInfo.pipelineLayout = pipelineLayouts[dePipeline::MRT];
			drawInfo.textureBindingSize = textureBindingDescriptor.size;
			objManager.DrawObjects(drawInfo);

			vkCmdEndRenderPass(cmdBuffer);
		}

		//Composition
		{
			clearValues[0].color = { 0,0,0,0 };
			clearValues[1].depthStencil = { 1.f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.renderArea.extent = {(uint32_t)window.viewport.width,
				(uint32_t)window.viewport.height};
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = swapChain.framebuffers[currentFrame].handle;

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineManager.Get(dePipeline::COMPOSITION));
			
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport = window.viewport;
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = window.scissor;
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

			// Binding 0 = image samplers
			std::array<VkDescriptorBufferBindingInfoEXT, 2> descriptor_buffer_binding_info = {};
			descriptor_buffer_binding_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[0].address =
				compositionImageBindingDescriptor.buffers[currentFrame].GetDeviceAddress();
			descriptor_buffer_binding_info[0].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			//Binding 1 = uniform light data
			descriptor_buffer_binding_info[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[1].address =
				uniformBindingDescriptors[dePipeline::COMPOSITION].buffers[currentFrame].GetDeviceAddress();
			descriptor_buffer_binding_info[1].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
			g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, descriptor_buffer_binding_info.size(),
				descriptor_buffer_binding_info.data());

			uint32_t buffer_index_images = 0;
			uint32_t buffer_index_ubo = 1;
			VkDeviceSize buffer_offset = 0;
			//frambuffer attachments.

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::COMPOSITION], 0, 1, &buffer_index_images, &buffer_offset);

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::COMPOSITION], 1, 1, &buffer_index_ubo, &buffer_offset);

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
		if (ContextBase::PrepareFrame())
		{ 
			UpdateScreenUniforms();
			UpdateLights();
			RecordCommandBuffers();
			ContextBase::SubmitFrame();
		}
		else 
		{
			ResizeWindow();
		}
	}

}