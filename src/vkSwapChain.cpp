#include "vkSwapChain.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <stdexcept>

namespace vk 
{
	SwapChain::SwapChain(Device* devicePtr, const std::array<uint32_t, 2>& queueFamilies, const vk::Window& appWindow) 
	{
		assert(devicePtr);

		this->devicePtr = devicePtr;

		VkSwapchainCreateInfoKHR swapChainInfo = {};
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = appWindow.surface;


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

		uint32_t imageCount = deviceCapabilities.minImageCount + 1;

		if (deviceCapabilities.maxImageCount > 0 && imageCount > deviceCapabilities.maxImageCount)
		{
			imageCount = deviceCapabilities.maxImageCount;
		}

		swapChainInfo.minImageCount = imageCount;
		swapChainInfo.imageColorSpace = surfaceFormats[surfaceIndex].colorSpace;
		swapChainInfo.imageFormat = surfaceFormats[surfaceIndex].format;
		swapChainInfo.imageExtent = deviceCapabilities.currentExtent;
		swapChainInfo.imageArrayLayers = 1;
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		//queueFamilies[0] == graphics, queueFamilies[1] == present
		if (queueFamilies[0] == queueFamilies[1])
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //present mode and graphics mode are the same.
			swapChainInfo.queueFamilyIndexCount = 0;
			swapChainInfo.pQueueFamilyIndices = nullptr;
		}
		else
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.queueFamilyIndexCount = queueFamilies.size();
			swapChainInfo.pQueueFamilyIndices = queueFamilies.data();
		}


		swapChainInfo.preTransform = deviceCapabilities.currentTransform;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.
		swapChainInfo.clipped = VK_TRUE;
		swapChainInfo.oldSwapchain = nullptr; //resizing needs a reference to the old swap chain


		this->createInfo = swapChainInfo;
		VK_CHECK_RESULT(vkCreateSwapchainKHR(devicePtr->logical, &swapChainInfo, nullptr, &this->handle));

		this->images.resize(imageCount);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(devicePtr->logical, this->handle, &imageCount, this->images.data()));

		SwapChain::CreateImageViews();

		this->depthAttachment = devicePtr->CreateFramebufferAttachment(appWindow.viewport, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	}
	
	void SwapChain::Recreate(const VkRenderPass renderPass, const vk::Window& appWindow)
	{
		SwapChain::Destroy();

		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devicePtr->physical, appWindow.surface, &deviceCapabilities));

		createInfo.imageExtent = deviceCapabilities.currentExtent;
		VK_CHECK_RESULT(vkCreateSwapchainKHR(devicePtr->logical, &createInfo, nullptr, &this->handle));

		this->images.resize(createInfo.minImageCount);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(devicePtr->logical, this->handle, &createInfo.minImageCount, this->images.data()));

		SwapChain::CreateImageViews();

		this->depthAttachment = devicePtr->CreateFramebufferAttachment(appWindow.viewport, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, this->depthAttachment.format);

		SwapChain::CreateFrameBuffers(appWindow.viewport, renderPass);
	}

	void SwapChain::Destroy() 
	{
		assert(devicePtr);

		for (unsigned i = 0; i < this->images.size(); ++i)
		{
			vkDestroyImageView(devicePtr->logical, this->imageViews[i], nullptr);
			vkDestroyFramebuffer(devicePtr->logical, this->frameBuffers[i], nullptr);
		}

		depthAttachment.Destroy(devicePtr->logical);

		vkDestroySwapchainKHR(devicePtr->logical, this->handle, nullptr);
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
		assert(vp.width != 0.f);
		assert(vp.height != 0.f);

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