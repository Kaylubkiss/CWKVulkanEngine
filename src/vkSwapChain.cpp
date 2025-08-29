#include "vkSwapChain.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <stdexcept>

namespace vk 
{
	SwapChain::SwapChain(const Device* devicePtr, uint32_t graphicsFamily, uint32_t presentFamily, const VkSurfaceKHR windowSurface)
	{
		assert(devicePtr);

		this->logicalDevice = devicePtr->logical;
		this->physicalDevice = devicePtr->physical;

		VkSwapchainCreateInfoKHR swapChainInfo = {};
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = windowSurface;


		uint32_t surfaceFormatCount = 0;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;

		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &surfaceFormatCount, nullptr));

		//surfaceFormatCount now filled..
		if (!surfaceFormatCount)
		{
			throw std::runtime_error("no surface formats available...");
		}

		surfaceFormats.resize(surfaceFormatCount);

		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &surfaceFormatCount, surfaceFormats.data()));


		
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
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &deviceCapabilities));

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

		uint32_t queueFamilyIndices[2] = { graphicsFamily, presentFamily };

		if (graphicsFamily == presentFamily)
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //present mode and graphics mode are the same.
			swapChainInfo.queueFamilyIndexCount = 0;
			swapChainInfo.pQueueFamilyIndices = nullptr;
		}
		else
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.queueFamilyIndexCount = 2;
			swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
		}


		swapChainInfo.preTransform = deviceCapabilities.currentTransform;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.
		swapChainInfo.clipped = VK_TRUE;
		swapChainInfo.oldSwapchain = nullptr; //resizing needs a reference to the old swap chain


		this->createInfo = swapChainInfo;
		VK_CHECK_RESULT(vkCreateSwapchainKHR(logicalDevice, &swapChainInfo, nullptr, &this->handle));

		this->images.resize(imageCount);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(logicalDevice, this->handle, &imageCount, this->images.data()));

		SwapChain::CreateImageViews(this->images.data(), (uint32_t)this->images.size());
	}
	
	void SwapChain::Recreate(vk::rsc::DepthStencil& depthResources, const VkRenderPass renderPass, const vk::Window& appWindow)
	{
		SwapChain::Destroy();

		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, appWindow.surface, &deviceCapabilities));

		createInfo.imageExtent = deviceCapabilities.currentExtent;
		VK_CHECK_RESULT(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &this->handle));

		this->images.resize(createInfo.minImageCount);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(logicalDevice, this->handle, &createInfo.minImageCount, this->images.data()));

		SwapChain::CreateImageViews(this->images.data(), (uint32_t)this->images.size());

		depthResources.Destroy(logicalDevice);
		depthResources = vk::rsc::CreateDepthResources(physicalDevice, logicalDevice, appWindow.viewport);

		SwapChain::AllocateFrameBuffers(appWindow.viewport, depthResources, renderPass);
	}

	void SwapChain::Destroy() 
	{
		assert(logicalDevice && physicalDevice);

		for (unsigned i = 0; i < this->images.size(); ++i)
		{
			vkDestroyImageView(logicalDevice, this->imageViews[i], nullptr);
			vkDestroyFramebuffer(logicalDevice, this->frameBuffers[i], nullptr);
		}

		vkDestroySwapchainKHR(logicalDevice, this->handle, nullptr);
	}

	void SwapChain::CreateImageViews(VkImage* images, uint32_t imageCount)
	{

		//create imageview --> allow image to be seen in a different format.
		this->imageViews.resize(imageCount);

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

		for (unsigned i = 0; i < imageCount; ++i) 
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

			VK_CHECK_RESULT(vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageViews[i]));
		}

	}

	void SwapChain::AllocateFrameBuffers(const VkViewport& vp, const vk::rsc::DepthStencil& depthResources, const VkRenderPass renderPass)
	{

		if (this->images.size() <= 0) 
		{
			throw std::runtime_error("have 0 swap chain images available. Did you allocate the swap chain?");
		}
		
		this->frameBuffers.resize(this->images.size());

		for (unsigned i = 0; i < this->images.size(); ++i) {

			VkImageView attachments[2] = { imageViews[i], depthResources.imageView };

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

			VK_CHECK_RESULT(vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &this->frameBuffers[i]));
		}

	}



}