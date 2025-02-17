#include "vkSwapChain.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <stdexcept>

namespace vk 
{
	SwapChain::SwapChain(const VkDevice l_device, const VkPhysicalDevice p_device, uint32_t graphicsFamily, uint32_t presentFamily, const VkSurfaceKHR windowSurface)
	{
		VkSwapchainCreateInfoKHR swapChainInfo = {};
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = windowSurface;


		uint32_t surfaceFormatCount = 0;
		VkSurfaceFormatKHR* surfaceFormats = nullptr;

		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, windowSurface, &surfaceFormatCount, nullptr));

		//surfaceFormatCount now filled..
		if (surfaceFormatCount <= 0)
		{
			throw std::runtime_error("no surface formats available...");
		}

		surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];

		if (surfaceFormats == nullptr)
		{
			throw std::runtime_error("failed to allocate surfaceFormats");
			return;
		}

		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, windowSurface, &surfaceFormatCount, surfaceFormats));


		//choose suitable format
		int surfaceIndex = -1;

		for (size_t i = 0; i < surfaceFormatCount; ++i)
		{
			if ((*(surfaceFormats + i)).format == VK_FORMAT_B8G8R8A8_SRGB && (*(surfaceFormats + i)).colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				surfaceIndex = i;
			}
		}

		if (surfaceIndex < 0)
		{
			surfaceIndex = 0;
		}

		if (surfaceIndex < 0)
		{
			throw std::runtime_error("couldn't find a suitable format for swap chain");
		}


		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, windowSurface, &deviceCapabilities));

		this->imageCount = deviceCapabilities.minImageCount + 1;

		if (deviceCapabilities.maxImageCount > 0 && this->imageCount > deviceCapabilities.maxImageCount)
		{
			this->imageCount = deviceCapabilities.maxImageCount;
		}

		swapChainInfo.minImageCount = this->imageCount;
		swapChainInfo.imageColorSpace = (*(surfaceFormats + surfaceIndex)).colorSpace;
		swapChainInfo.imageFormat = (*(surfaceFormats + surfaceIndex)).format;
		swapChainInfo.imageExtent = deviceCapabilities.currentExtent;
		swapChainInfo.imageArrayLayers = 1;
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		if (graphicsFamily == presentFamily)
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //present mode and graphics mode are the same.
			swapChainInfo.queueFamilyIndexCount = 0;
			swapChainInfo.pQueueFamilyIndices = nullptr;
		}
		else
		{
			uint32_t queueFamilyIndices[2] = { graphicsFamily, presentFamily };

			swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.queueFamilyIndexCount = 2;
			swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
		}


		swapChainInfo.preTransform = deviceCapabilities.currentTransform;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.
		swapChainInfo.clipped = VK_TRUE;
		swapChainInfo.oldSwapchain = nullptr; //resizing needs a reference to the old swap chain


		VK_CHECK_RESULT(vkCreateSwapchainKHR(l_device, &swapChainInfo, nullptr, &this->handle));

		delete[] surfaceFormats;


		this->images = new VkImage[this->imageCount];

		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(l_device, this->handle, &this->imageCount, this->images));

		//Do some extra work to setup the framebuffers.
		SwapChain::CreateImageViews(l_device, this->images, this->imageCount);
	}
	
	void SwapChain::Recreate(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t graphicsFamily, uint32_t presentFamily, const DepthResources& depthResources, const VkRenderPass renderPass, const vk::Window& appWindow)
	{
		//reason we don't call destroy: don't need to destroy every resource in swapchain.

		SwapChain::Destroy(l_device);

		*this = SwapChain(l_device, p_device, graphicsFamily, presentFamily, appWindow.surface);

		SwapChain::CreateImageViews(l_device, this->images, this->imageCount);
	
		SwapChain::AllocateFrameBuffers(l_device, appWindow.viewport, depthResources, renderPass);
	}

	void SwapChain::CreateImageViews(const VkDevice l_device, VkImage* images, uint32_t imageCount)
	{

		//create imageview --> allow image to be seen in a different format.
		this->imageViews = new VkImageView[imageCount];

		for (unsigned i = 0; i < imageCount; ++i) 
		{
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

			VkImageViewCreateInfo imageViewCreateInfo =
			{
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				nullptr, //pNext
				0, //flags
				images[i], //the created image above
				VK_IMAGE_VIEW_TYPE_2D, //view image type
				VK_FORMAT_B8G8R8A8_SRGB, //as long as the same bits per pixel, the parent and view will be	compatible.
				componentMapping,
				subresourceRange
			};

			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(l_device, &imageViewCreateInfo, nullptr, &imageViews[i]));
		}

	}

	void SwapChain::AllocateFrameBuffers(const VkDevice l_device, const VkViewport& vp, const DepthResources& depthResources, const VkRenderPass renderPass)
	{

		if (this->imageCount <= 0) 
		{
			throw std::runtime_error("have 0 swap chain images available. Did you allocate the swap chain?");
		}
		
		this->frameBuffers = new VkFramebuffer[this->imageCount];

		for (unsigned i = 0; i < this->imageCount; ++i) {

			VkImageView attachments[2] = { imageViews[i], depthResources.depthImageView };

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

			VK_CHECK_RESULT(vkCreateFramebuffer(l_device, &framebufferCreateInfo, nullptr, &this->frameBuffers[i]));
		}

	}



}