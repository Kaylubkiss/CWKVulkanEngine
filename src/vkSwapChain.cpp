#include "vkSwapChain.h"
#include "vkUtility.h"
#include <stdexcept>

namespace vk 
{
	SwapChain CreateSwapChain(const VkDevice l_device, const VkPhysicalDevice p_device, uint32_t graphicsFamily, uint32_t presentFamily, const VkSurfaceKHR windowSurface)
	{
		SwapChain nSwapChain;

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

		nSwapChain.imageCount = deviceCapabilities.minImageCount + 1;

		if (deviceCapabilities.maxImageCount > 0 && nSwapChain.imageCount > deviceCapabilities.maxImageCount)
		{
			nSwapChain.imageCount = deviceCapabilities.maxImageCount;
		}

		swapChainInfo.minImageCount = nSwapChain.imageCount;
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


		VK_CHECK_RESULT(vkCreateSwapchainKHR(l_device, &swapChainInfo, nullptr, &nSwapChain.handle));

		delete[] surfaceFormats;
		
		nSwapChain.images = new VkImage[nSwapChain.imageCount];

		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(l_device, nSwapChain.handle, &nSwapChain.imageCount, nSwapChain.images));

		return nSwapChain;
	}


}