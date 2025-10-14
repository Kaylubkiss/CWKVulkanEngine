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

		uint32_t imageCount = deviceCapabilities.minImageCount < 2 ? 2 : deviceCapabilities.minImageCount;
		
		if (deviceCapabilities.maxImageCount > 0 && imageCount > deviceCapabilities.maxImageCount)
		{
			imageCount = deviceCapabilities.maxImageCount;
		}

		createInfo.minImageCount = imageCount;
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

		createInfo.preTransform = deviceCapabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = oldSwapchain; //resizing needs a reference to the old swap chain


		VK_CHECK_RESULT(vkCreateSwapchainKHR(devicePtr->logical, &createInfo, nullptr, &this->handle));

		if (oldSwapchain != VK_NULL_HANDLE)
		{
			for (unsigned i = 0; i < this->images.size(); ++i)
			{
				vkDestroyImageView(devicePtr->logical, this->imageViews[i], nullptr);
				imageViews[i] = VK_NULL_HANDLE;
			}

			this->depthAttachment.Destroy(devicePtr->logical);

			vkDestroySwapchainKHR(devicePtr->logical, oldSwapchain, nullptr);
		}
		
		this->images.resize(imageCount);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(devicePtr->logical, this->handle, &imageCount, this->images.data()));

		SwapChain::CreateImageViews();

		this->depthAttachment = devicePtr->CreateFramebufferAttachment(appWindow.viewport, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);		

	}
	
	void SwapChain::Recreate(const VkRenderPass renderPass, const vk::Window& appWindow)
	{
		SwapChain::Create(appWindow);

		for (auto& framebuffer : frameBuffers) 
		{
			vkDestroyFramebuffer(devicePtr->logical, framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}

		SwapChain::CreateFrameBuffers(appWindow.viewport, renderPass);

	}

	void SwapChain::Destroy() 
	{
		assert(devicePtr);

		for (unsigned i = 0; i < this->images.size(); ++i)
		{
			vkDestroyImageView(devicePtr->logical, this->imageViews[i], nullptr);
			imageViews[i] = VK_NULL_HANDLE;
			vkDestroyFramebuffer(devicePtr->logical, this->frameBuffers[i], nullptr);
			frameBuffers[i] = VK_NULL_HANDLE;
		}

		depthAttachment.Destroy(devicePtr->logical);

		vkDestroySwapchainKHR(devicePtr->logical, this->handle, nullptr);
		handle = VK_NULL_HANDLE;
	}

	void SwapChain::CreateImageViews()
	{
		assert(this->images.empty() == false);

		//create imageview --> allow image to be seen in a different format.
		this->imageViews.resize(this->images.size());

		//this is nothing fancy, we won't be editing the color interpretation.
		VkComponentMapping componentMapping =
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
		};

		//the view can only refer to one aspect of the parent image.
		VkImageSubresourceRange subresourceRange =
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, //base mip level
			1, //levelCount for mip levels
			0, //baseArrayLayer -> layer not an array image
			1, //layerCount for image array. 
		};

		for (unsigned i = 0; i < this->images.size(); ++i) 
		{
			VkImageViewCreateInfo imageViewCreateInfo =
			{
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				nullptr, //pNext
				0, //flags
				images[i], //the created image above
				VK_IMAGE_VIEW_TYPE_2D, //view image type
				createInfo.imageFormat, //as long as the same bits per pixel, the parent and view will compatible.
				componentMapping,
				subresourceRange
			};

			VK_CHECK_RESULT(vkCreateImageView(devicePtr->logical, &imageViewCreateInfo, nullptr, &imageViews[i]));
		}

	}

	void SwapChain::CreateFrameBuffers(const VkViewport& vp, const VkRenderPass renderPass)
	{

		assert(this->depthAttachment.imageView != VK_NULL_HANDLE);
		assert(renderPass != VK_NULL_HANDLE);

		if (this->images.size() <= 0) 
		{
			throw std::runtime_error("have 0 swap chain images available. Did you allocate the swap chain?");
		}
		
		this->frameBuffers.resize(this->images.size());

		for (unsigned i = 0; i < this->images.size(); ++i) {

			VkImageView attachments[2] = { imageViews[i], this->depthAttachment.imageView };

			//create framebuffer info
			VkFramebufferCreateInfo framebufferCreateInfo =
			{
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				nullptr, //pNext
				0, //reserved for future expansion.. flags are zero now.
				renderPass,
				2,// attachmentCount
				attachments, //attachments
				static_cast<uint32_t>(vp.width), //width
				static_cast<uint32_t>(vp.height), //height
				1 //1 layer
			};

			VK_CHECK_RESULT(vkCreateFramebuffer(devicePtr->logical, &framebufferCreateInfo, nullptr, &this->frameBuffers[i]));
		}

	}



}