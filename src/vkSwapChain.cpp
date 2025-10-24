#include "vkSwapChain.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <stdexcept>

namespace vk 
{
	void SwapChain::Init(vk::Device* devicePtr, const vk::Window& appWindow)
	{
		assert(devicePtr);

		this->devicePtr = devicePtr;

		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = appWindow.surface;

		uint32_t surfaceFormatCount = 0;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(devicePtr->physical, appWindow.surface, &surfaceFormatCount, nullptr));

		//surfaceFormatCount now filled..
		if (!surfaceFormatCount)
		{
			throw std::runtime_error("no surface formats available...");
		}

		surfaceFormats.resize(surfaceFormatCount);

		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(devicePtr->physical, appWindow.surface, &surfaceFormatCount, surfaceFormats.data()));

		//choose suitable format
		int surfaceIndex = 0;
		for (size_t i = 0; i < surfaceFormats.size(); ++i)
		{
			if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
				surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
				surfaceFormats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
			{
				surfaceIndex = i;
				break;
			}
		}


		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devicePtr->physical, appWindow.surface, &deviceCapabilities));

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

	}

	void SwapChain::Create(const vk::Window& appWindow) 
	{
		assert(this->devicePtr);

		VkSwapchainKHR oldSwapchain = this->handle;


		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devicePtr->physical, appWindow.surface, &deviceCapabilities));

		uint32_t desiredImageCount = deviceCapabilities.minImageCount < 2 ? 2 : deviceCapabilities.minImageCount;
		if (deviceCapabilities.maxImageCount > 0 && desiredImageCount > deviceCapabilities.maxImageCount)
		{
			desiredImageCount = deviceCapabilities.maxImageCount;
		}

		createInfo.surface = appWindow.surface;
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

		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = oldSwapchain; //resizing needs a reference to the old swap chain


		VK_CHECK_RESULT(vkCreateSwapchainKHR(devicePtr->logical, &createInfo, nullptr, &this->handle));

		if (oldSwapchain != VK_NULL_HANDLE)
		{
			for (auto& framebuffer : framebuffers)
			{
				framebuffer.Destroy();
			}

			vkDestroySwapchainKHR(devicePtr->logical, oldSwapchain, nullptr);
			oldSwapchain = VK_NULL_HANDLE;
		}
		
		uint32_t imageCount = 0;
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(devicePtr->logical, this->handle, &imageCount, nullptr));

		this->images.resize(imageCount);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(devicePtr->logical, this->handle, &imageCount, this->images.data()));
	}
	
	void SwapChain::Recreate(const VkRenderPass renderPass, const vk::Window& appWindow)
	{
		SwapChain::Create(appWindow);
		SwapChain::CreateFrameBuffers(appWindow.viewport, renderPass);
	}

	void SwapChain::Destroy() 
	{
		assert(devicePtr);

		for (auto& framebuffer : framebuffers)
		{
			framebuffer.Destroy();
		}

		vkDestroySwapchainKHR(devicePtr->logical, this->handle, nullptr);
		handle = VK_NULL_HANDLE;
	}

	void SwapChain::CreateFrameBuffers(const VkViewport& vp, const VkRenderPass renderPass)
	{
		assert(renderPass != VK_NULL_HANDLE);
		
		this->framebuffers.resize(this->images.size());

		vk::FramebufferAttachmentCreateInfo attachmentInfo = {};
		attachmentInfo.width = static_cast<uint32_t>(vp.width);
		attachmentInfo.height = static_cast<uint32_t>(vp.height);

		VkExtent2D windowExtent = { attachmentInfo.width, attachmentInfo.height };

		for (unsigned i = 0; i < this->images.size(); ++i) 
		{
			framebuffers[i].Init(this->devicePtr, windowExtent);

			attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			attachmentInfo.format = createInfo.imageFormat;
			attachmentInfo.alreadyAllocatedImage = images[i];

			framebuffers[i].AddAttachment(attachmentInfo);

			attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			attachmentInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
			attachmentInfo.alreadyAllocatedImage = VK_NULL_HANDLE;

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
				windowExtent.width, //width
				windowExtent.height, //height
				1 //1 layer
			};

			VK_CHECK_RESULT(vkCreateFramebuffer(devicePtr->logical, &framebufferCreateInfo, nullptr, &this->framebuffers[i].handle));
		}

	}
}