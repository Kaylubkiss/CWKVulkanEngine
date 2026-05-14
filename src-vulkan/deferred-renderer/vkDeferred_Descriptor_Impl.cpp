#include "vkDeferredRenderer.h"
namespace vk
{


    void DeferredRenderer::InitializeDescriptors( DescriptorManager& descriptorManager )
    {
        {
        	InitializeUBODescriptors(descriptorManager);

        	InitializeMaterialDescriptors(descriptorManager);

        	skyboxImageIndex = m_descriptorManagerPtr->GetLayoutIndex(DescriptorCategory::eMaterial);

        	vk::imageBuffers2D panoramicImageBuffer;
        	panoramicImageBuffer.resize(1);
        	panoramicImageBuffer[0].push_back(m_test_panoramicImage.GetCubeMapImageDescriptor());

        	m_descriptorManagerPtr->WriteDescriptors(DescriptorCategory::eMaterial,
				skyboxImageIndex, panoramicImageBuffer);

        	InitializeCompositionImageDescriptors(descriptorManager);

		}
    }


	void DeferredRenderer::InitializeUBODescriptors( DescriptorManager& descriptorManager )
	{
    	//there are three ubos in this demo.
    	//1 - scene transforms: eye, projection
    	//2 - shadow projection
    	//3 - lighting struct
    	//..not in that order.
	    size_t ubo_count = 3;

	    std::vector<VkDescriptorSetLayoutBinding> bindings(1);
	    bindings[0].binding = 0;
	    bindings[0].descriptorCount = 1;
	    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

	    descriptorManager.AllocateDescriptorBuffer(DescriptorCategory::eUBO, gMaxFramesInFlight, ubo_count, bindings);

	    mrtUBOLayoutIndex = descriptorManager.GetLayoutIndex(DescriptorCategory::eUBO);
	    {
		    vk::resourceBufferPtrs2D resourceBufferPtrs;
		    resourceBufferPtrs.resize(gMaxFramesInFlight);
		    for (size_t frame = 0; frame < resourceBufferPtrs.size(); ++frame)
		    {
			    vk::Buffer* handle = &uniformBuffers[frame].mrt;
			    resourceBufferPtrs[frame].push_back(handle);
		    }

		    descriptorManager.WriteDescriptors(DescriptorCategory::eUBO, mrtUBOLayoutIndex, resourceBufferPtrs);
	    }

	    shadowUBOLayoutIndex = descriptorManager.GetLayoutIndex(DescriptorCategory::eUBO);
	    {
		    vk::resourceBufferPtrs2D resourceBufferPtrs;
		    resourceBufferPtrs.resize(gMaxFramesInFlight);
		    for (size_t frame = 0; frame < resourceBufferPtrs.size(); ++frame)
		    {
			    vk::Buffer* handle = &uniformBuffers[frame].shadow;

			    resourceBufferPtrs[frame].push_back(handle);
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

	void DeferredRenderer::InitializeCompositionImageDescriptors( DescriptorManager& descriptorManager )
    {
    	size_t imageCount = RT_COUNT + 1 + 1; //+shadow, +irradiance_map

    	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings(imageCount);

    	for (uint32_t i = 0; i < setLayoutBindings.size(); ++i)
    	{
    		setLayoutBindings[i].binding = i;
    		setLayoutBindings[i].descriptorCount = 1;
    		setLayoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    		setLayoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    	}

		descriptorManager.AllocateDescriptorBuffer(DescriptorCategory::eCompositionImage,
			gMaxFramesInFlight, imageCount, setLayoutBindings);

    	compositionImageIndex = m_descriptorManagerPtr->GetLayoutIndex(DescriptorCategory::eCompositionImage);

    	vk::imageBuffers2D imageDescriptorData;
    	imageDescriptorData.resize(gMaxFramesInFlight);

    	for (size_t frame = 0; frame < imageDescriptorData.size(); ++frame)
    	{
    		imageDescriptorData[frame].resize(imageCount);

    		for (size_t binding = 0; binding < RT_COUNT; ++binding)
    		{
    			imageDescriptorData[frame][binding].imageLayout = framebuffers.deMRT[frame].attachments[binding].layout;
    			imageDescriptorData[frame][binding].imageView = framebuffers.deMRT[frame].attachments[binding].imageView;
    			imageDescriptorData[frame][binding].sampler = framebuffers.deMRT[frame].sampler;
    		}

    		imageDescriptorData[frame][RT_COUNT].imageLayout = framebuffers.deShadow[frame].attachments[0].layout;
    		imageDescriptorData[frame][RT_COUNT].imageView = framebuffers.deShadow[frame].attachments[0].imageView;
    		imageDescriptorData[frame][RT_COUNT].sampler = framebuffers.deShadow[frame].sampler;

    		VkDescriptorImageInfo irradianceMapDescriptor = m_test_panoramicImage.GetIrradianceImageDescriptor();
		    imageDescriptorData[frame][RT_COUNT + 1].imageLayout = irradianceMapDescriptor.imageLayout;
		    imageDescriptorData[frame][RT_COUNT + 1].imageView = irradianceMapDescriptor.imageView;
    		imageDescriptorData[frame][RT_COUNT + 1].sampler = irradianceMapDescriptor.sampler;
	    }

    	descriptorManager.WriteDescriptors(DescriptorCategory::eCompositionImage, compositionImageIndex, imageDescriptorData);

    	swapChainImageIndex = m_descriptorManagerPtr->GetLayoutIndex(DescriptorCategory::eCompositionImage);

    	imageDescriptorData.clear();
    	imageDescriptorData.resize(gMaxFramesInFlight);

    	for (size_t frame = 0; frame < imageDescriptorData.size(); ++frame)
    	{
    		VkDescriptorImageInfo swapchain_image_info = {};
    		//image view and sampler should be identical across framebuffers
    		swapchain_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    		swapchain_image_info.imageView = framebuffers.deSky[frame].attachments[0].imageView;
    		swapchain_image_info.sampler = framebuffers.deSky[frame].sampler;

    		imageDescriptorData[frame].push_back(swapchain_image_info);
    	}

    	descriptorManager.WriteDescriptors(DescriptorCategory::eCompositionImage, swapChainImageIndex, imageDescriptorData);
    }

	void DeferredRenderer::InitializeMaterialDescriptors( DescriptorManager& descriptorManager )
    {
    	//texture material samplers
    	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings(3);

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

		descriptorManager.AllocateDescriptorBuffer(DescriptorCategory::eMaterial, 1,
			OBJECT_COUNT, setLayoutBindings);

    	//texture manager will fill the descriptor buffers.
    }
}