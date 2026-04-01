#include "vkDeferredShadingContext.h"

namespace vk
{


    void DeferredContext::InitializeDescriptors( DescriptorManager& descriptorManager  )
    {
        {

        	descriptorManager.Init(&device);

        	//there are three ubos in this demo.
        	//1 - scene transforms: eye, projection
        	//2 - shadow projection
        	//3 - lighting struct
        	//..not in that order.{
        	InitializeUBODescriptors(descriptorManager);

			//MRT uniform descriptors
			//InitializeMRTDescriptor();

			InitializeCompositionSamplerDescriptor();

        	//InitializeCompositionUniformDescriptor();

			//Shadow map uniform descriptor
			// InitializeShadowMapDescriptor();

			//Skybox sampler descriptor
			InitializeSkyBoxDescriptor();

			//Swapchain sampler descriptor --> this needs the skybox descriptors!
			InitializeSwapChainDescriptor();
		}
    }


	void DeferredContext::InitializeUBODescriptors( DescriptorManager& descriptorManager )
	{
	    {
		    size_t ubo_count = 3;
	    	size_t layoutCount = ubo_count * gMaxFramesInFlight;

	    	std::vector<VkDescriptorSetLayoutBinding> bindings(1);
	    	bindings[0].binding = 0;
	    	bindings[0].descriptorCount = 1;
	    	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

	    	descriptorManager.AllocateDescriptorBuffer(DescriptorCategory::eUBO, ubo_count, layoutCount, bindings);

	    	mrtUBOLayoutIndex = descriptorManager.GetLayoutIndex(DescriptorCategory::eUBO);
		    {
		    	vk::resourceBufferPtrs2D resourceBufferPtrs;
		    	resourceBufferPtrs.resize(gMaxFramesInFlight);
		    	for (size_t frame = 0; frame < resourceBufferPtrs.size(); ++frame)
		    	{
		    		resourceBufferPtrs[frame].push_back(&uniformBuffers[frame].mrt);
		    	}

		    	descriptorManager.WriteDescriptors(DescriptorCategory::eUBO, mrtUBOLayoutIndex, resourceBufferPtrs);
		    }

	    	shadowUBOLayoutIndex = descriptorManager.GetLayoutIndex(DescriptorCategory::eUBO);
		    {
		    	vk::resourceBufferPtrs2D resourceBufferPtrs;
		    	resourceBufferPtrs.resize(gMaxFramesInFlight);
		    	for (size_t frame = 0; frame < resourceBufferPtrs.size(); ++frame)
		    	{
		    		resourceBufferPtrs[frame].push_back(&uniformBuffers[frame].shadow);
		    	}

		    	descriptorManager.WriteDescriptors(DescriptorCategory::eUBO, shadowUBOLayoutIndex, resourceBufferPtrs);
		    }

	    	lightUBOLayoutIndex = descriptorManager.GetLayoutIndex(DescriptorCategory::eUBO);
		    {
		    	vk::resourceBufferPtrs2D resourceBufferPtrs;
		    	resourceBufferPtrs.resize(gMaxFramesInFlight);
		    	for (size_t frame = 0; frame < resourceBufferPtrs.size(); ++frame)
		    	{
		    		resourceBufferPtrs[frame].push_back(&uniformBuffers[frame].composition);
		    	}

		    	descriptorManager.WriteDescriptors(DescriptorCategory::eUBO, lightUBOLayoutIndex, resourceBufferPtrs);
		    }
	    }
    }

	void DeferredContext::InitializeCompositionSamplerDescriptor()
    {
    	std::array<VkDescriptorSetLayoutBinding, 6> setLayoutBindings = {};

    	//set 0: per-frame image resources
    	for (uint32_t i = 0; i < setLayoutBindings.size(); ++i)
    	{
    		setLayoutBindings[i].binding = i;
    		setLayoutBindings[i].descriptorCount = 1;
    		setLayoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    		setLayoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    	}

    	DescriptorBufferCreateInfo descriptorBufferCI = {};
    	descriptorBufferCI.devicePtr = &device;
    	descriptorBufferCI.bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
			VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    	descriptorBufferCI.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    	descriptorBufferCI.pLayoutBindings = setLayoutBindings.data();
    	descriptorBufferCI.layoutBindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    	descriptorBufferCI.imageDescriptorData.resize(gMaxFramesInFlight);
    	for (size_t frame = 0; frame < gMaxFramesInFlight; ++frame)
    	{
    		auto& imageInfos =
				descriptorBufferCI.imageDescriptorData[frame];

    		imageInfos.resize(RT_COUNT + 1);
    		for (size_t binding = 0; binding < RT_COUNT; ++binding)
    		{
    			imageInfos[binding].imageLayout = framebuffers.deMRT[frame].attachments[binding].layout;
    			imageInfos[binding].imageView = framebuffers.deMRT[frame].attachments[binding].imageView;
    			imageInfos[binding].sampler = framebuffers.deMRT[frame].sampler;
    		}

    		imageInfos[RT_COUNT].imageLayout = framebuffers.deShadow[frame].attachments[0].layout;
    		imageInfos[RT_COUNT].imageView = framebuffers.deShadow[frame].attachments[0].imageView;
    		imageInfos[RT_COUNT].sampler = framebuffers.deShadow[frame].sampler;
    	}

    	compositionImageBindingDescriptor.Create(descriptorBufferCI);
    }

	void DeferredContext::InitializeCompositionUniformDescriptor()
	{
    	std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings = {};
    	setLayoutBindings[0].descriptorCount = 1;
    	setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    	setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    	DescriptorBufferCreateInfo descriptorBufferCI = {};
    	descriptorBufferCI.devicePtr = &device;
    	descriptorBufferCI.bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    	descriptorBufferCI.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    	descriptorBufferCI.pLayoutBindings = setLayoutBindings.data();
    	descriptorBufferCI.layoutBindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    	descriptorBufferCI.resourceDescriptorData.resize(gMaxFramesInFlight);
    	for (size_t frame = 0; frame < gMaxFramesInFlight; ++frame)
    	{
    		descriptorBufferCI.resourceDescriptorData[frame].push_back(&uniformBuffers[frame].composition);
    	}

    	uniformBindingDescriptors[dePipeline::COMPOSITION].Create(descriptorBufferCI);
    }

	void DeferredContext::InitializeSwapChainDescriptor()
	{
    	std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings = {};
    	setLayoutBindings.front().descriptorCount = 1;
    	setLayoutBindings.front().descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    	setLayoutBindings.front().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    	DescriptorBufferCreateInfo descriptorBufferCI = {};
    	descriptorBufferCI.devicePtr = &device;
    	descriptorBufferCI.pLayoutBindings = setLayoutBindings.data();
    	descriptorBufferCI.layoutBindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    	descriptorBufferCI.bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
		VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    	descriptorBufferCI.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    	descriptorBufferCI.imageDescriptorData.resize(gMaxFramesInFlight);
    	for (size_t frame = 0; frame < gMaxFramesInFlight; ++frame)
    	{
    		VkDescriptorImageInfo swapchain_image_info = {};
    		//image view and sampler should be identical across framebuffers
    		swapchain_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    		swapchain_image_info.imageView = framebuffers.deSky[frame].attachments[0].imageView;
    		swapchain_image_info.sampler = framebuffers.deSky[frame].sampler;

    		descriptorBufferCI.imageDescriptorData[frame].push_back(swapchain_image_info);
    	}

    	swapChainSamplerBindingDescriptor.Create(descriptorBufferCI);
    }

	void DeferredContext::InitializeMRTDescriptor()
	{
    	std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings = {};
    	//per-frame scene transform (ubo)
    	setLayoutBindings.front() = {};
    	setLayoutBindings.front().binding = 0;
    	setLayoutBindings.front().descriptorCount = 1;
    	setLayoutBindings.front().descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    	setLayoutBindings.front().stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    	DescriptorBufferCreateInfo descriptorBufferCI = {};
    	descriptorBufferCI.devicePtr = &device;
    	descriptorBufferCI.bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    	descriptorBufferCI.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    	descriptorBufferCI.pLayoutBindings = setLayoutBindings.data();
    	descriptorBufferCI.layoutBindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    	descriptorBufferCI.resourceDescriptorData.resize(gMaxFramesInFlight);
    	for (size_t frame = 0; frame < gMaxFramesInFlight; ++frame)
    	{
    		descriptorBufferCI.resourceDescriptorData[frame].push_back(&uniformBuffers[frame].mrt);
    	}

    	uniformBindingDescriptors[dePipeline::MRT].Create(descriptorBufferCI);
    }

	void DeferredContext::InitializeShadowMapDescriptor()
	{
    	std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings = {};
    	setLayoutBindings.front().descriptorCount = 1;
    	setLayoutBindings.front().descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    	setLayoutBindings.front().stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

    	DescriptorBufferCreateInfo descriptorBufferCI = {};
    	descriptorBufferCI.devicePtr = &device;
    	descriptorBufferCI.bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    	descriptorBufferCI.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    	descriptorBufferCI.pLayoutBindings = setLayoutBindings.data();
    	descriptorBufferCI.layoutBindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    	descriptorBufferCI.resourceDescriptorData.resize(gMaxFramesInFlight);
    	for (size_t frame = 0; frame < gMaxFramesInFlight; ++frame)
    	{
    		descriptorBufferCI.resourceDescriptorData[frame].push_back(&uniformBuffers[frame].shadow);
    	}

    	uniformBindingDescriptors[dePipeline::SHADOW].Create(descriptorBufferCI);
    }

	void DeferredContext::InitializeSkyBoxDescriptor()
	{
    	std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings = {};
    	setLayoutBindings.front().descriptorCount = 1;
    	setLayoutBindings.front().descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    	setLayoutBindings.front().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    	DescriptorBufferCreateInfo descriptorBufferCI = {};
    	descriptorBufferCI.devicePtr = &device;
    	descriptorBufferCI.pLayoutBindings = setLayoutBindings.data();
    	descriptorBufferCI.layoutBindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    	descriptorBufferCI.bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
		VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    	descriptorBufferCI.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    	VkDescriptorImageInfo skyboxTextureInfo = test_cube.GetDescriptor();

    	descriptorBufferCI.imageDescriptorData.push_back({skyboxTextureInfo});

    	skyboxSamplerBindingDescriptor.Create(descriptorBufferCI);
    }
}