#include "Application.h"
#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "vkUtility.h"
#include "vkDebug.h"
#include "vkInit.h"


//NOTE: to remove pesky warnings from visual studio, on dynamically allocated arrays,
//I've used the syntax: *(array + i) to access the array instead of array[i].
//the static analyzer of visual studio is bad.


static const std::string shaderPath{ "Shaders/" };

const static glm::vec4 X_BASIS = { 1,0,0,0 };
const static glm::vec4 Y_BASIS = { 0,1,0,0 };
const static glm::vec4 Z_BASIS = { 0,0,1,0 };
const static glm::vec4 W_BASIS = { 0,0,0,1 };

void Application::UpdateUniformViewMatrix() 
{
	if (mCamera.isUpdated()) 
	{
		uTransform.view = mCamera.LookAt();
		memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));
	}
}

Camera& Application::GetCamera()
{
	return this->mCamera;
}

void Application::run() 
{
	//initialize all resources.
	init();

	//render, update, render, update...
	loop();

	//cleanup resources
	exit();
}

void Application::ToggleObjectVisibility(SDL_Keycode keysym, uint8_t lshift) 
{
	/*debugCube3.debugDrawObject.ToggleVisibility(keysym, lshift);
	debugCube2.debugDrawObject.ToggleVisibility(keysym, lshift);*/

}

void Application::CreateWindow(vk::Window& appWindow)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}

	appWindow.sdl_ptr = SDL_CreateWindow("Caleb's Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
	int(appWindow.viewport.width), int(appWindow.viewport.height), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (appWindow.sdl_ptr == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
	}


	
}

bool Application::WindowisFocused() 
{
	if (this->mWindow.sdl_ptr == NULL) 
	{
		return false;
	}

	uint32_t flags = SDL_GetWindowFlags(this->mWindow.sdl_ptr);

	return ((flags & SDL_WINDOW_INPUT_FOCUS) != 0);

}

void Application::CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow) 
{
	if (SDL_Vulkan_CreateSurface(appWindow.sdl_ptr, vkInstance, &appWindow.surface) != SDL_TRUE)
	{
		throw std::runtime_error("could not create window surface!");
	}
}

void Application::CreateSwapChain() 
{

	VkSwapchainCreateInfoKHR swapChainInfo = {};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.surface = this->m_windowSurface;

	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_physicalDevices[device_index], m_windowSurface, &this->deviceCapabilities));


	uint32_t surfaceFormatCount = 0;
	VkSurfaceFormatKHR* surfaceFormats = nullptr;

	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_physicalDevices[device_index], this->m_windowSurface, &surfaceFormatCount, nullptr));

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

	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_physicalDevices[device_index], this->m_windowSurface, &surfaceFormatCount, surfaceFormats));


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


	this->imageCount = this->deviceCapabilities.minImageCount + 1;

	if (deviceCapabilities.maxImageCount > 0 && imageCount > deviceCapabilities.maxImageCount) 
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
	

	VK_CHECK_RESULT(vkCreateSwapchainKHR(this->m_logicalDevice, &swapChainInfo, nullptr, &this->swapChain));

	delete [] surfaceFormats;

}

void Application::CreateImageViews()
{

	this->swapChainImages = new VkImage[imageCount];

	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(this->m_logicalDevice, this->swapChain, &this->imageCount, this->swapChainImages));

	//create imageview --> allow image to be seen in a different format.
	this->imageViews = new VkImageView[imageCount];

	for (unsigned i = 0; i < this->imageCount; ++i) {

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
			swapChainImages[i], //the created image above
			VK_IMAGE_VIEW_TYPE_2D, //view image type
			VK_FORMAT_B8G8R8A8_SRGB, //as long as the same bits per pixel, the parent and view will be compatible.
			componentMapping,
			subresourceRange
		};

		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT(vkCreateImageView(this->m_logicalDevice, &imageViewCreateInfo, nullptr, &imageViews[i]));

		

	}

}

void Application::CreateFrameBuffers() 
{

	this->frameBuffer = new VkFramebuffer[imageCount];

	for (unsigned i = 0; i < imageCount; ++i) {

		VkImageView attachments[2] = {this->imageViews[i], this->depthImageView};

		//create framebuffer info
		VkFramebufferCreateInfo framebufferCreateInfo =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr, //pNext
			0, //reserved for future expansion.. flags are zero now.
			this->m_renderPass,
			2,// attachmentCount
			attachments, //attachments
			(uint32_t)this->mWindowExtents.width, //width
			(uint32_t)this->mWindowExtents.height, //height
			1 //1 layer
		};

		VK_CHECK_RESULT(vkCreateFramebuffer(this->m_logicalDevice, &framebufferCreateInfo, nullptr, &this->frameBuffer[i]));
	}

}


void Application::CreateUniformBuffers()
{
	this->uniformBuffers.push_back(Buffer(sizeof(uTransformObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)&uTransform));
	
}



static VkCommandBuffer beginCmd(const VkDevice& l_device, const VkCommandPool& cmdPool) 
{

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(l_device, &allocInfo, &cmdBuffer));

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

	return cmdBuffer;

}

static void endCmd(VkCommandBuffer commandBuffer, const VkCommandPool& cmdPool, const VkDevice& l_device, const VkQueue& gfxQueue)
{
	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK_RESULT(vkQueueWaitIdle(gfxQueue));

	vkFreeCommandBuffers(l_device, cmdPool, 1, &commandBuffer);

}

static void TransitionImageLayout(VkImage image, VkFormat format,
								  VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) 
{
	VkCommandBuffer cmdBuffer = beginCmd();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;
	
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else 
	{
		throw std::invalid_argument("bad layout transition");
	}

	//first two parameters 
	vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	endCmd(cmdBuffer);

}

static void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) 
{
	VkCommandBuffer cmdBuffer = beginCmd();

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0,0,0 };
	region.imageExtent = 
	{
		width,
		height,
		1 
	};

	vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endCmd(cmdBuffer);
}

static std::string PathToTextures() 
{
	return "External/textures/";
}



void Application::GenerateMipMaps(const VkPhysicalDevice& p_device, VkImage image, VkFormat imgFormat, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels) 
{

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(p_device, imgFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
	{
		throw std::runtime_error("your physical device does not support linear blitting");
		//TODO: generate mipmap levels with software/storing mip levels in texture image and sampling that.
	}

	VkCommandBuffer cmdBuffer = beginCmd();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	uint32_t mipWidth = textureWidth;
	uint32_t mipHeight = textureHeight;

	for (uint32_t i = 1; i < mipLevels; i++) 
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 
		nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0,0,0 };
		blit.srcOffsets[1] = { (int)(mipWidth), (int)(mipHeight), 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		
		blit.dstOffsets[0] = { 0,0,0 };
		blit.dstOffsets[1] = { (int)(mipWidth > 1 ? mipWidth / 2 : 1), (int)(mipHeight > 1 ? mipHeight / 2 : 1), 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmdBuffer, 
			image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, 
			&blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);


		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;

	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr, 0, nullptr, 1, &barrier);
	
	endCmd(cmdBuffer);

}


void Application::CreateTexture(const std::string& fileName)
{
	//TODO: ensure that the same texture isn't allocated twice.
	
	this->mTextures.push_back(Texture());
	Texture& newTexture = this->mTextures.back();

	int textureWidth, textureHeight, textureChannels;
	stbi_uc* pixels = stbi_load((PathToTextures() + fileName).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	
	VkDeviceSize imageSize = textureWidth * textureHeight * 4;

	if (pixels == NULL) 
	{
		throw std::runtime_error("failed to load texture image!");
	}

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;
	
	Buffer stagingBuffer = Buffer(static_cast<size_t>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pixels);

	stbi_image_free(pixels);

	CreateImage(textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.mTextureImage, newTexture.mTextureMemory, 1);

	TransitionImageLayout(newTexture.mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	
	copyBufferToImage(stagingBuffer.handle, newTexture.mTextureImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight));
	
	GenerateMipMaps(newTexture.mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, (uint32_t)textureWidth, (uint32_t)textureHeight, mipLevels);

	vkDestroyBuffer(this->m_logicalDevice, stagingBuffer.handle, nullptr);
	vkFreeMemory(this->m_logicalDevice, stagingBuffer.memory, nullptr);

	CreateTextureView(newTexture.mTextureImage, newTexture.mTextureImageView, mipLevels);

	CreateTextureSampler(newTexture.mTextureSampler, mipLevels);

	this->mTextures.back().mName = fileName;
}

void Application::CreateTextureView(const VkImage& textureImage, VkImageView& textureImageView, uint32_t mipLevels)
{

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	
	VK_CHECK_RESULT(vkCreateImageView(this->m_logicalDevice, &viewInfo, nullptr, &textureImageView));
}

void Application::CreateTextureSampler(VkSampler& textureSampler, uint32_t mipLevels)
{
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	createInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties pdp = { };
	vkGetPhysicalDeviceProperties(this->m_physicalDevices[device_index], &pdp);

	createInfo.maxAnisotropy = pdp.limits.maxSamplerAnisotropy / 2.f;

	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;

	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.minLod = 0.f;
	createInfo.maxLod = static_cast<float>(mipLevels);
	createInfo.mipLodBias = 0.f; //optional...

	VK_CHECK_RESULT(vkCreateSampler(this->m_logicalDevice, &createInfo, nullptr, &textureSampler));
}


const VkPipeline& Application::GetTrianglePipeline()
{
	return this->pipeline;
}

const VkPipeline& Application::GetLinePipeline()
{
	return this->linePipeline;
}

VkPipelineLayout* Application::GetPipelineLayout() 
{
	return &(this->pipelineLayouts.back());

}

void Application::CreateDescriptorSets()
{
	//create descriptor pool
	VkDescriptorPoolSize poolSize[2] = {};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 2; //max numbers of frames in flight.

	//we are concerned about the fragment stage, so we double the descriptor count here.
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 1 * 2; //max numbers of frames in flight times two to accomodate the gui.

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = (uint32_t)2;
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = mTextures.size() > 0? mTextures.size() * 2 : 1; //max numbers of frames in flight.

	VK_CHECK_RESULT(vkCreateDescriptorPool(this->m_logicalDevice, &poolInfo, nullptr, &this->descriptorPool));

	for (size_t i = 0; i < mTextures.size(); ++i) 
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.descriptorPool = this->descriptorPool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &this->descriptorSetLayout;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(this->m_logicalDevice, &descriptorAllocInfo, &this->mTextures[i].mDescriptor));

	}
}

void Application::WriteDescriptorSets() 
{
	VkDescriptorBufferInfo uTransformbufferInfo = {};
	uTransformbufferInfo.buffer = uniformBuffers[0].handle;
	uTransformbufferInfo.offset = 0;
	uTransformbufferInfo.range = sizeof(uTransformObject);

	VkDescriptorBufferInfo uLightInfoBufferInfo = {};
	uLightInfoBufferInfo.buffer = mLights.mBuffer.handle;
	uLightInfoBufferInfo.offset = 0;
	uLightInfoBufferInfo.range = sizeof(LightInfoObject) - sizeof(int) * LightCountIndex::MAX_IND_COUNT;

	VkDescriptorBufferInfo bufferInfo[2] = { uTransformbufferInfo, uLightInfoBufferInfo };
	VkDescriptorImageInfo imageInfo = {};

	for (size_t i = 0; i < this->mTextures.size(); ++i) 
	{
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = this->mTextures[i].mTextureImageView;
		imageInfo.sampler = this->mTextures[i].mTextureSampler;

		VkWriteDescriptorSet descriptorWrite[3] = {};
		descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[0].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[0].dstBinding = 0;
		descriptorWrite[0].dstArrayElement = 0;
		descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[0].descriptorCount = 1; //how many buffers
		descriptorWrite[0].pBufferInfo = &uTransformbufferInfo;
		descriptorWrite[0].pImageInfo = nullptr; // Optional
		descriptorWrite[0].pTexelBufferView = nullptr; // Optional

		descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[1].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[1].dstBinding = 1;
		descriptorWrite[1].dstArrayElement = 0;
		descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[1].descriptorCount = 1; //how many buffers
		descriptorWrite[1].pBufferInfo = &uLightInfoBufferInfo;
		descriptorWrite[1].pImageInfo = nullptr; // Optional
		descriptorWrite[1].pTexelBufferView = nullptr; // Optional

		descriptorWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[2].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[2].dstBinding = 2;
		descriptorWrite[2].dstArrayElement = 0;
		descriptorWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite[2].descriptorCount = 1; //how many images
		descriptorWrite[2].pImageInfo = &imageInfo;
		descriptorWrite[2].pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(this->m_logicalDevice, 3, descriptorWrite, 0, nullptr);
	}

}

void Application::CreatePipelineLayout() 
{
	VkPushConstantRange pushConstants[1];

	//this is for an object's model transformation.
	pushConstants[0].offset = 0;
	pushConstants[0].size = sizeof(glm::mat4);
	pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo				pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstants;

	this->pipelineLayouts.push_back(VkPipelineLayout());

	VK_CHECK_RESULT(vkCreatePipelineLayout(this->m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &this->pipelineLayouts.back()));


}

void Application::CreatePipeline(VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology, VkPipeline& pipelineHandle)
{
	auto vAttribs = vk::init::VertexAttributeDescriptions();

	VkVertexInputBindingDescription vBindingDescription = {};
	vBindingDescription.stride = sizeof(struct Vertex);
	vBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::init::VertexInputStateCreateInfo();
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1; //vertexBindingDescriptionCount
	vertexInputCreateInfo.pVertexBindingDescriptions = &vBindingDescription,
	vertexInputCreateInfo.vertexAttributeDescriptionCount = vAttribs.size(); //attribute count
	vertexInputCreateInfo.pVertexAttributeDescriptions = vAttribs.data();

	VkPipelineInputAssemblyStateCreateInfo pipelineAssemblyCreateInfo = vk::init::AssemblyInputStateCreateInfo(primitiveTopology);

	VkDynamicState dynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 2;
	dynamicStateCreateInfo.pDynamicStates = dynamicState;

	
	VkPipelineViewportStateCreateInfo viewPortCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		1, //viewportCount
		nullptr, //pViewPorts
		1, //scissorCount
		nullptr, //pScissors
	};


	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		VK_FALSE, //depthClampEnable
		VK_FALSE, //rasterizerDiscardEnable
		VK_POLYGON_MODE_FILL, //polygonMode
		VK_CULL_MODE_BACK_BIT, //cullMode
		VK_FRONT_FACE_COUNTER_CLOCKWISE, //frontFace
		VK_FALSE, //depthBiasEnable
		0.f, //depthBiasConstantFactor
		0.f, //depthBiasClamp
		0.f, //depthBiasSlopeFactor
		1.f, //lineWidth
	};

	VkPipelineColorBlendAttachmentState			colorBlendAttachState;
	colorBlendAttachState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachState.blendEnable = VK_FALSE;
	colorBlendAttachState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	colorBlendAttachState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	colorBlendAttachState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachState.alphaBlendOp = VK_BLEND_OP_ADD;



	VkPipelineColorBlendStateCreateInfo			colorBlendCreateInfo;
	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendCreateInfo.pNext = nullptr;
	colorBlendCreateInfo.flags = 0;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;

	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachState;
	colorBlendCreateInfo.blendConstants[0] = 0;
	colorBlendCreateInfo.blendConstants[1] = 0;
	colorBlendCreateInfo.blendConstants[2] = 0;
	colorBlendCreateInfo.blendConstants[3] = 0;


	//depthstencil testing
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.minDepthBounds = 0.f;
	depthStencilCreateInfo.maxDepthBounds = 1.f;
	
	//no stencil test for now.
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilCreateInfo.front = {};
	depthStencilCreateInfo.back = {};

	
	VkPipelineMultisampleStateCreateInfo multiSampleCreateInfo = {};
	multiSampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multiSampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	VkGraphicsPipelineCreateInfo gfxPipelineCreateInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		(uint32_t)numStages,
		pStages,
		//TODO: VkPipelineVertexInputStateCreateInfo
		&vertexInputCreateInfo,
		//TODO: VkPipelineInputAssemblyStateCreateInfo,
		&pipelineAssemblyCreateInfo,
		//TODO: VkPipelineTessellationStateCreateInfo,
		nullptr,
		//TODO: VkPipelineViewportStateCreateInfo,
		&viewPortCreateInfo,
		//TODO: VkPipelineRasterizationStateCreateInfo,
		&rasterizationStateCreateInfo,
		//TODO: VkPipelineMultisampleStateCreateInfo,
		&multiSampleCreateInfo,
		//TODO: VkPipelineDepthStencilStateCreateInfo,
		&depthStencilCreateInfo,
		//TODO: VkPipelineColorBlendStateCreateInfo,
		&colorBlendCreateInfo,
		//VkPipelineDynamicStateCreateInfo,
		&dynamicStateCreateInfo,
		//VkPipelineLayout,
		this->pipelineLayouts.back(),
		//VkRenderPass,
		this->m_renderPass,
		//subpass,
		0,
		//basePipelineHandle,
		VK_NULL_HANDLE,
		//basePipelineIndex   
		0
	};

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(this->m_logicalDevice, VK_NULL_HANDLE, 1, &gfxPipelineCreateInfo, nullptr, &pipelineHandle));

}

void Application::CreateCommandPools(const VkDevice& l_device)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //recording commands every frame.
	commandPoolCreateInfo.queueFamilyIndex = 0; //only one physical device on initial development machine.

	VK_CHECK_RESULT(vkCreateCommandPool(l_device, &commandPoolCreateInfo, nullptr, &this->commandPool));
}

void Application::CreateCommandBuffers() 
{
	VkCommandBufferAllocateInfo cmdBufferCreateInfo = {};

	cmdBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferCreateInfo.commandPool = this->commandPool;
	cmdBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //cannot be called by other command buffers
	cmdBufferCreateInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(this->m_logicalDevice, &cmdBufferCreateInfo, &this->commandBuffer));
}

void Application::CreateSemaphores() 
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK_RESULT(vkCreateSemaphore(this->m_logicalDevice, &semaphoreInfo, nullptr, &this->imageAvailableSemaphore));

	VK_CHECK_RESULT(vkCreateSemaphore(this->m_logicalDevice, &semaphoreInfo, nullptr, &this->renderFinishedSemaphore));

}

void Application::CreateFences() 
{
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //to prevent indefinite waiting on first frame.

	VK_CHECK_RESULT(vkCreateFence(this->m_logicalDevice, &fenceInfo, nullptr, &this->inFlightFence));
}

void Application::RecreateSwapChain() 
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(this->m_logicalDevice));

	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_physicalDevices[device_index], this->m_windowSurface, &this->deviceCapabilities));

	this->mWindowExtents.width = deviceCapabilities.currentExtent.width;
	this->mWindowExtents.height = deviceCapabilities.currentExtent.height;

	for (unsigned i = 0; i < this->imageCount; ++i)
	{
		vkDestroyImageView(this->m_logicalDevice, this->imageViews[i], nullptr);
		vkDestroyFramebuffer(this->m_logicalDevice, this->frameBuffer[i], nullptr);
	}

	
	vkDestroyImageView(this->m_logicalDevice, this->depthImageView, nullptr);
	vkDestroyImage(this->m_logicalDevice, this->depthImage, nullptr);
	
	delete[] this->frameBuffer;
	delete[] this->swapChainImages;

	vkDestroySwapchainKHR(this->m_logicalDevice, this->swapChain, nullptr);

	CreateSwapChain();

	CreateImageViews();

	CreateDepthResources();

	CreateFrameBuffers();
}


Physics& Application::PhysicsSystem() 
{
	return this->mPhysics;
}


void Application::ResizeViewport(VkViewport& vp, SDL_Window* windowHandle)
{
	int nWidth, nHeight;
	SDL_GetWindowSizeInPixels(windowHandle, &nWidth, &nHeight);
	vp.width = (float)nWidth;
	vp.height = (float)nHeight;
	uTransform.proj = glm::perspective(glm::radians(45.f), (float)vp.width / vp.height, 0.1f, 1000.f); //proj
	uTransform.proj[1][1] *= -1.f;
	memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));

}

void Application::InitPhysicsWorld() 
{
	this->mObjectManager["cube"].InitPhysics(ColliderType::CUBE);
	
	reactphysics3d::Material& db2Material = this->mObjectManager["cube"].mPhysicsComponent.collider->getMaterial();
	db2Material.setBounciness(0.f);
	db2Material.setMassDensity(10.f);
	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->updateMassPropertiesFromColliders();
	
	this->mObjectManager["base"].InitPhysics(ColliderType::CUBE, BodyType::STATIC);
	
	mCamera.InitPhysics(BodyType::STATIC);


	this->mObjectManager["cube"].SetLinesArrayOffset(12);

	//this->mPhysicsWorld->setIsDebugRenderingEnabled(true);
	this->mPhysics.GetPhysicsWorld()->setIsDebugRenderingEnabled(true);

	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->setIsDebugEnabled(true);
	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->setIsDebugEnabled(true);
	
	//the order they were added to the physics world
	reactphysics3d::DebugRenderer& debugRenderer = this->mPhysics.GetPhysicsWorld()->getDebugRenderer();
	debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);
	
}

void Application::InitGui() 
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	// Setup Platform/Renderer backends
	if (!ImGui_ImplSDL2_InitForVulkan(this->window)) {

		throw std::runtime_error("couldn't initialize imgui for vulkan!!!\n");
		return;
	}


	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = this->m_instance;
	init_info.PhysicalDevice = this->m_physicalDevices[device_index];
	init_info.Device = this->m_logicalDevice;
	init_info.QueueFamily = this->graphicsFamily;
	init_info.Queue = this->graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = this->descriptorPool;
	init_info.RenderPass = this->m_renderPass;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = this->imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = vkutil::check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);
}


int Application::GetTexture(const char* fileName) 
{
	for (size_t i = 0; i < mTextures.size(); ++i) 
	{
		if (strcmp(fileName, mTextures[i].mName.c_str()) == 0) 
		{
			return i;
		}
	}

	this->CreateTexture(std::string(fileName));
	return this->mTextures.size() - 1;
}

bool Application::init() 
{
	this->mCamera = Camera({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f } , { 0,1,0 });

	this->mWindow.viewport.width = 640;
	this->mWindow.viewport.height = 480;

	this->uTransform = {
		glm::mat4(1.f), //model
		this->mCamera.LookAt(), //view
		glm::perspective(glm::radians(45.f), (float)this->mWindow.viewport.width / this->mWindow.viewport.height, 0.1f, 1000.f) //proj
	};


	this->uTransform.proj[1][1] *= -1.f;


	CreateWindow(this->mWindow);

	m_instance = vk::init::CreateInstance(this->mWindow.sdl_ptr);

	CreateWindowSurface();

	//setup the debug callbacks... (optional...)

	EnumeratePhysicalDevices();
	
	//retrieve queue family properties 
	// --> group of queues that have identical capabilities and are able to run in parallel 
	//		--> could be arithmetic, passing shaders, stuff like that.
	/*FindQueueFamilies();*/

	CreateLogicalDevice();

	glm::mat4 modelTransform = glm::mat4(5.f);
	modelTransform[3] = glm::vec4(1.f, 0, -20.f, 1);


	mObjectManager.LoadObject("freddy", "freddy.obj", false, modelTransform);

	//object 2
	modelTransform = glm::mat4(1.f);
	modelTransform[3] = glm::vec4(0, 20, -5.f, 1);

	mObjectManager.LoadObject("cube", "cube.obj", true, modelTransform);

	//object 3
	const float dbScale = 30.f;
	modelTransform = glm::mat4(dbScale);
	modelTransform[3] = { 0.f, -5.f, 0.f, 1 };

	mObjectManager.LoadObject("base", "base.obj", true, modelTransform);

	vkGetDeviceQueue(this->m_logicalDevice, graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(this->m_logicalDevice, presentFamily, 0, &presentQueue);

	
	// If you want to draw a triangle:
	// - create renderpass object
	CreateSwapChain();

	CreateImageViews();

	std::string vertexShaderPath = shaderPath + "blinnvert.spv";
	VkPipelineShaderStageCreateInfo shaderVertStageInfo = vkutil::CreateShaderModule(this->m_logicalDevice, vertexShaderPath.data(), this->shaderVertModule, VK_SHADER_STAGE_VERTEX_BIT);

	std::string fragShaderPath = shaderPath + "blinnfrag.spv";
	VkPipelineShaderStageCreateInfo shaderFragModuleInfo = vkutil::CreateShaderModule(this->m_logicalDevice, fragShaderPath.data(), this->shaderFragModule, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineShaderStageCreateInfo shaderStages[] = { shaderVertStageInfo, shaderFragModuleInfo };

	////create layout 
	CreateCommandPools();
	CreateCommandBuffers();
	
	CreateUniformBuffers();


	mLights.Create({ 0, 10, 0 }, { 0, -1, 0 });
	
	CreateDepthResources();

	//ERROR: didn't create descriptoryLayout!!!

	CreatePipelineLayout();
	
	CreateRenderPass();
	CreateFrameBuffers();
	
	CreatePipeline(shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, this->pipeline);
	CreatePipeline(shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, this->linePipeline);

	//commands
	CreateSemaphores();

	CreateFences();

	
	mTime = Time(SDL_GetPerformanceCounter());

	while (mObjectManager.mThreadWorkers.isBusy()) { //wait until the jobs are done... 
	}

	this->mObjectManager["freddy"].UpdateTexture("texture.jpg");
	this->mObjectManager["freddy"].UpdatePipelineLayout(&this->pipelineLayouts.back());

	this->mObjectManager["cube"].UpdateTexture("puppy1.bmp");
	this->mObjectManager["cube"].UpdatePipelineLayout(&this->pipelineLayouts.back());

	this->mObjectManager["base"].UpdateTexture("puppy1.bmp");
	this->mObjectManager["base"].UpdatePipelineLayout(&this->pipelineLayouts.back());


	CreateDescriptorSets();
	WriteDescriptorSets();


	InitGui();

	InitPhysicsWorld();

	//ERROR: vksetcmdviewport? scissor? you made a dynamic viewport!!!


	return true;


}

SDL_Window* Application::GetWindow() const
{
	return this->window;
}

const Time& Application::GetTime()
{
	return this->mTime;
}

class RayCastObject : public RaycastCallback {
public:
	virtual decimal notifyRaycastHit(const RaycastInfo& info)
	{
		// Display the world hit point coordinates
		std::cout << " Hit point : " <<
			info.worldPoint.x <<
			info.worldPoint.y <<
			info.worldPoint.z <<
			std::endl;

		// Return a fraction of 1.0 to gather all hits
		return decimal(-1.0);
	}
};


void Application::SelectWorldObjects(const int& mouseX, const int& mouseY)
{
	

	glm::vec4 cursorWindowPos(mouseX, mouseY, 1, 1);

	glm::vec4 cursorScreenPos = {};

	//ndc
	cursorScreenPos.x = (2 * cursorWindowPos.x) / this->mWindowExtents.width - 1;
	cursorScreenPos.y = 1 - (2 * cursorWindowPos.y) / this->mWindowExtents.height; //vulkan is upside down.
	cursorScreenPos.z = -1;
	cursorScreenPos.w = 1;

	////eye

	////world 
	glm::vec4 ray_world = glm::inverse(uTransform.view * uTransform.proj) * cursorScreenPos;

	ray_world /= ray_world.w;

	//2. cast ray from the mouse position and in the direction forward from the mouse position

	glm::vec3 CameraPos = mCamera.Position();

	reactphysics3d::Vector3 rayStart(CameraPos.x, CameraPos.y, CameraPos.z);

	reactphysics3d::Vector3 rayEnd(ray_world.x, ray_world.y, ray_world.z);

	Ray ray(rayStart, rayEnd);

	RaycastInfo raycastInfo = {};

	RayCastObject callbackObject;

	this->mPhysics.GetPhysicsWorld()->raycast(ray, &callbackObject);

}

void Application::DrawGui() 
{

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + main_viewport->WorkSize.x / 15, main_viewport->WorkPos.y + main_viewport->WorkSize.y / 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x / 3, main_viewport->WorkSize.y / 2), ImGuiCond_Once);


	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_MenuBar;
	// Main body of the Demo window starts here.
	if (!ImGui::Begin("Asset Log", nullptr, window_flags))
	{
		// Early out if the window is collapsed, as an optimization
		guiWindowIsFocused = ImGui::IsWindowFocused();
		ImGui::End();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), this->commandBuffer);
		return;
	}

	guiWindowIsFocused = ImGui::IsWindowFocused();
	
	ImGui::End();
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), this->commandBuffer);


}

void Application::Render() 
{
	
	VK_CHECK_RESULT(vkWaitForFences(this->m_logicalDevice, 1, &this->inFlightFence, VK_TRUE, UINT64_MAX))
	VK_CHECK_RESULT(vkResetFences(this->m_logicalDevice, 1, &this->inFlightFence))

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(this->m_logicalDevice, this->swapChain, UINT64_MAX, this->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);


	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		ResizeViewport();
		return;
	}

	assert(result == VK_SUCCESS);

	VK_CHECK_RESULT(vkResetCommandBuffer(this->commandBuffer, 0))


	////always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
	//always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	//everything else is default...

	VK_CHECK_RESULT(vkBeginCommandBuffer(this->commandBuffer, &cmdBufferBeginInfo))

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = this->m_renderPass;
	renderPassInfo.framebuffer = this->frameBuffer[imageIndex];
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = deviceCapabilities.currentExtent;

	VkClearValue clearColors[2] = {};
	clearColors[0].color = { {0.f, 0.f, 0.f, 1.f} };
	clearColors[1].depthStencil = { 1.f, 0 };

	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearColors;

	//put this in a draw frame
	vkCmdBeginRenderPass(this->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//bind the graphics pipeline
	vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

	/*vkCmdSetViewport(this->commandBuffer, 0, 1, &this->m_viewPort);
	vkCmdSetScissor(this->commandBuffer, 0, 1, &this->m_scissor);*/

	
	this->mObjectManager["freddy"].Draw(this->commandBuffer);
	this->mObjectManager["base"].Draw(this->commandBuffer);  
	this->mObjectManager["cube"].Draw(this->commandBuffer);


	DrawGui();

	vkCmdEndRenderPass(this->commandBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(this->commandBuffer));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->commandBuffer;

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, this->inFlightFence));

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
		ResizeViewport();
		return;
	}

	assert(result == VK_SUCCESS);

}


void Application::RequestExit() 
{
	this->exitApplication = true;
}


void Application::loop()
{
	//render graphics.
	while (exitApplication == false)
	{	
		mTime.Update();

		mController.Update();

		mPhysics.Update(mTime.DeltaTime());

		this->mObjectManager["cube"].Update(mPhysics.InterpFactor());
		this->mObjectManager["base"].Update(mPhysics.InterpFactor());
		
		mCamera.Update(mPhysics.InterpFactor());

		mLights.Update();
		

		Render();
	}

	VK_CHECK_RESULT(vkDeviceWaitIdle(this->m_logicalDevice));



}

const std::vector<Texture>& Application::Textures() 
{
	return this->mTextures;

}


void Application::DestroyObjects() 
{
	/*debugCube.DestroyResources();
	debugCube2.DestroyResources();
	debugCube3.DestroyResources();*/
}

void Application::exit()
{

	CleanUpGui();

	DestroyObjects();

	vkDestroySemaphore(this->m_logicalDevice, this->imageAvailableSemaphore, nullptr);

	vkDestroySemaphore(this->m_logicalDevice, this->renderFinishedSemaphore, nullptr);

	vkFreeCommandBuffers(this->m_logicalDevice, this->commandPool, 1, &this->commandBuffer);
	
	vkDestroyCommandPool(this->m_logicalDevice, this->commandPool, nullptr);

	for (size_t i = 0; i < this->pipelineLayouts.size(); ++i) 
	{
		vkDestroyPipelineLayout(this->m_logicalDevice, this->pipelineLayouts[i], nullptr);
	}

	
	vkDestroyPipeline(this->m_logicalDevice, this->pipeline, nullptr);

	vkDestroyPipeline(this->m_logicalDevice, this->linePipeline, nullptr);

	vkDestroyDescriptorPool(this->m_logicalDevice, this->descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(this->m_logicalDevice, this->descriptorSetLayout, nullptr);

	for (unsigned i = 0; i < uniformBuffers.size(); ++i) 
	{
		vkDestroyBuffer(this->m_logicalDevice, uniformBuffers[i].handle, nullptr);
		vkFreeMemory(this->m_logicalDevice, uniformBuffers[i].memory, nullptr);
	}

	for (size_t i = 0; i < this->mTextures.size(); ++i) 
	{
		vkDestroySampler(this->m_logicalDevice, this->mTextures[i].mTextureSampler, nullptr);
		vkDestroyImageView(this->m_logicalDevice, this->mTextures[i].mTextureImageView, nullptr);
		vkDestroyImage(this->m_logicalDevice, this->mTextures[i].mTextureImage, nullptr);
		vkFreeMemory(this->m_logicalDevice, this->mTextures[i].mTextureMemory, nullptr);
	}


	vkDestroyImage(this->m_logicalDevice, this->depthImage, nullptr);
	vkDestroyImageView(this->m_logicalDevice, this->depthImageView, nullptr);
	vkFreeMemory(this->m_logicalDevice, this->depthImageMemory, nullptr);

	for (unsigned i = 0; i < imageCount; ++i)
	{
		vkDestroyImageView(this->m_logicalDevice, this->imageViews[i], nullptr);
		vkDestroyFramebuffer(this->m_logicalDevice, this->frameBuffer[i], nullptr);
	}

	vkDestroyShaderModule(this->m_logicalDevice, this->shaderVertModule, nullptr);

	vkDestroyShaderModule(this->m_logicalDevice, this->shaderFragModule, nullptr);
	
	vkDestroyRenderPass(this->m_logicalDevice, this->m_renderPass, nullptr);

	//this already destroys the images in it.
	//vkDestroySwapchainKHR(this->m_logicalDevice, this->swapChain, nullptr);

	vkDestroyFence(this->m_logicalDevice, this->inFlightFence, nullptr);


	//delete[] swapChainImages;
	//delete[] m_physicalDevices;
	
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->m_instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) 
	{
		func(this->m_instance, this->debugMessenger, nullptr);
	}

	mLights.Deallocate();

	mObjectManager.Deallocate();

}


Application::~Application()
{
	/*vkDestroyDevice(this->m_logicalDevice, nullptr);*/

	vkDestroySurfaceKHR(this->m_instance, this->mWindow.surface, nullptr);

	vkDestroyInstance(this->m_instance, nullptr);
	
}


void Application::CleanUpGui() 
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}





