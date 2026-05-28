#include "vkSwapChain.h"
#include "vkUtil.h"
#include <stdexcept>

namespace vk 
{
	void SwapChain::Init(vk::Device* devicePtr, const vk::Window& appWindow)
	{
		assert(devicePtr);

		m_devicePtr = devicePtr;

		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = appWindow.Surface();

		uint32_t surfaceFormatCount = 0;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_devicePtr->GetGPU(), appWindow.Surface(), &surfaceFormatCount, nullptr));

		//surfaceFormatCount now filled..
		if (!surfaceFormatCount)
		{
			throw std::runtime_error("no surface formats available...");
		}

		surfaceFormats.resize(surfaceFormatCount);

		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_devicePtr->GetGPU(), appWindow.Surface(), &surfaceFormatCount, surfaceFormats.data()));

		//choose suitable format
		int surfaceIndex = 0;
		for (size_t i = 0; i < surfaceFormats.size(); ++i)
		{
			if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
				surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
				surfaceFormats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
			{
				surfaceIndex = static_cast<int>(i);
				break;
			}
		}


		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_devicePtr->GetGPU(), appWindow.Surface(), &deviceCapabilities));

		uint32_t imageCount = deviceCapabilities.minImageCount < 2 ? 2 : deviceCapabilities.minImageCount;

		if (deviceCapabilities.maxImageCount > 0 && imageCount > deviceCapabilities.maxImageCount)
		{
			imageCount = deviceCapabilities.maxImageCount;
		}

		createInfo.imageColorSpace = surfaceFormats[surfaceIndex].colorSpace;
		createInfo.imageFormat = surfaceFormats[surfaceIndex].format;


		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //present mode and graphics mode are the same.
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

		CreateRenderPass();
	}

	void SwapChain::Create( const vk::Window& appWindow )
	{
		assert(m_devicePtr);

		VkSwapchainKHR oldSwapchain = this->handle;

		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_devicePtr->GetGPU(), appWindow.Surface(), &deviceCapabilities));

		uint32_t desiredImageCount = deviceCapabilities.minImageCount < 2 ? 2 : deviceCapabilities.minImageCount;
		if (deviceCapabilities.maxImageCount > 0 && desiredImageCount > deviceCapabilities.maxImageCount)
		{
			desiredImageCount = deviceCapabilities.maxImageCount;
		}

		createInfo.surface = appWindow.Surface();
		createInfo.minImageCount = desiredImageCount;
		createInfo.imageExtent = deviceCapabilities.currentExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		if (deviceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) 
		{
			createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if (deviceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) 
		{
			createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		if (deviceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {

			createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else 
		{
			createInfo.preTransform = deviceCapabilities.currentTransform;
		}

		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		std::array<VkCompositeAlphaFlagBitsKHR, 4> compositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};

		for (auto& compositeAlphaFlag : compositeAlphaFlags) {

			if (deviceCapabilities.supportedCompositeAlpha & compositeAlphaFlag) 
			{
				createInfo.compositeAlpha = compositeAlphaFlag;
				break;
			}
		}

		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.+
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = oldSwapchain; //resizing needs a reference to the old swap chain

		VK_CHECK_RESULT(vkCreateSwapchainKHR(m_devicePtr->GetDevice(), &createInfo, nullptr, &this->handle));

		if (oldSwapchain != VK_NULL_HANDLE)
		{
			for (auto& framebuffer : framebuffers)
			{
				framebuffer.Destroy();
			}

			vkDestroySwapchainKHR(m_devicePtr->GetDevice(), oldSwapchain, nullptr);
			oldSwapchain = VK_NULL_HANDLE;
			images.clear();
		}
		
		uint32_t imageCount = 0;
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_devicePtr->GetDevice(), this->handle, &imageCount, nullptr));

		this->images.resize(imageCount);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_devicePtr->GetDevice(), this->handle, &imageCount, this->images.data()));

		VkCommandBuffer commandBuffer = m_devicePtr->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		//transition the image layouts for presentation.
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = images[i];
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		m_devicePtr->FlushCommandBuffer(commandBuffer, m_devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle, true);

		CreateFrameBuffers(appWindow.Viewport());
	}
	
	void SwapChain::Recreate( const vk::Window& appWindow )
	{
		SwapChain::Create( appWindow );
	}

	void SwapChain::Destroy() 
	{
		if (m_devicePtr != nullptr)
		{
			for (auto& framebuffer : framebuffers)
			{
				framebuffer.Destroy();
			}

			vkDestroyRenderPass(m_devicePtr->GetDevice(), renderPass, nullptr);

			vkDestroySwapchainKHR(m_devicePtr->GetDevice(), this->handle, nullptr);
			handle = VK_NULL_HANDLE;
		}
	}

	void SwapChain::CreateRenderPass()
	{
		assert( createInfo.imageFormat != VK_FORMAT_UNDEFINED );

		std::array<VkAttachmentDescription, 1> attachments = {};

		//color attachment
		attachments[0].format = createInfo.imageFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;

		//for layout transitions
		std::array<VkSubpassDependency, 2> dependencies{};

		//color writing/reading dependencies.
		//This is to ensure that the color attachment read/writes are finished before subpass 0 begins and uses them again for reading/writing.
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstStageMask = dependencies[0].srcStageMask;
		dependencies[0].srcAccessMask = 0; //this can also be 0
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //this can also be 0
		dependencies[1].dstAccessMask = 0;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassCI = {};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassCI.pAttachments = attachments.data();
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpassDescription;
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassCI.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(m_devicePtr->GetDevice(), &renderPassCI, nullptr, &renderPass));
	}

	void SwapChain::CreateFrameBuffers( const VkViewport& vp )
	{
		assert( renderPass != VK_NULL_HANDLE );

		this->framebuffers.clear();
		this->framebuffers.resize(this->images.size());

		uint32_t width = static_cast<uint32_t>(vp.width);
		uint32_t height = static_cast<uint32_t>(vp.height);

		vk::FramebufferAttachmentCreateInfo attachmentInfo = {};
		attachmentInfo.width = width;
		attachmentInfo.height = height;
		attachmentInfo.layerCount = 1;
		attachmentInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		attachmentInfo.operatingLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		for (unsigned i = 0; i < this->images.size(); ++i) 
		{
			framebuffers[i].Init(m_devicePtr);
			framebuffers[i].width = width;
			framebuffers[i].height = height;

			attachmentInfo.format = createInfo.imageFormat;
			attachmentInfo.alreadyAllocatedImage = images[i];

			framebuffers[i].AddAttachment(attachmentInfo);

			std::vector<VkImageView> imageViews(framebuffers[i].attachments.size());
			for (size_t j = 0; j < framebuffers[i].attachments.size(); ++j)
			{
				imageViews[j] = framebuffers[i].attachments[j].imageView;
			}

			//create framebuffer info
			VkFramebufferCreateInfo framebufferCreateInfo =
			{
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				nullptr, //pNext
				0, //reserved for future expansion.. flags are zero now.
				renderPass,
				static_cast<uint32_t>(imageViews.size()),// attachmentCount
				imageViews.data(), //attachments
				framebuffers[i].width, //width
				framebuffers[i].height, //height
				1 //1 layer
			};

			VK_CHECK_RESULT(vkCreateFramebuffer(m_devicePtr->GetDevice(), &framebufferCreateInfo,
				nullptr, &this->framebuffers[i].handle));
		}


	}



}