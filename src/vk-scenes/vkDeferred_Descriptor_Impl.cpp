#include "vkDeferredShadingContext.h"

namespace vk
{
    void DeferredContext::InitializeDescriptorBuffers()
    {
        {
			//MRT uniform descriptors
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

			//Scene textures descriptor
			{
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

				//NOTE: buffer size must be multiplied by OBJECT_COUNT
				textureSamplerBindingDescriptor.Create(descriptorBufferCI);
			}

			//Composition sampler descriptor
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
					for (size_t rt = 0; rt < RT_COUNT; ++rt)
					{
						imageInfos[rt].imageLayout = framebuffers.deMRT.attachments[rt].layout;
						imageInfos[rt].imageView = framebuffers.deMRT.attachments[rt].imageView;
						imageInfos[rt].sampler = framebuffers.deMRT.sampler;
					}

					imageInfos[RT_COUNT].imageLayout = framebuffers.deShadow.attachments[0].layout;
					imageInfos[RT_COUNT].imageView = framebuffers.deShadow.attachments[0].imageView;
					imageInfos[RT_COUNT].sampler = framebuffers.deShadow.sampler;
				}

				compositionImageBindingDescriptor.Create(descriptorBufferCI);
			}

			//Composition uniform descriptor
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

			//Shadow map uniform descriptor
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

			//Skybox sampler descriptor
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

			//Swapchain sampler descriptor
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
					swapchain_image_info.imageView = framebuffers.deSky.attachments[0].imageView;
					swapchain_image_info.sampler = framebuffers.deSky.sampler;

					descriptorBufferCI.imageDescriptorData[frame].push_back(swapchain_image_info);
				}

				swapChainSamplerBindingDescriptor.Create(descriptorBufferCI);
			}
		}

    }
}