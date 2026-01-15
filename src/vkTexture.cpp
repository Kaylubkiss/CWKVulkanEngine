#include "vkTexture.h"
#include "vkUtility.h"
#include <stb_image.h>

namespace vk {


	VkImageView Texture::CreateImageView(const VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels)
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

		VkImageView nTextImageView;
		VK_CHECK_RESULT(vkCreateImageView(l_device, &viewInfo, nullptr, &nTextImageView));

		return nTextImageView;
	}

	VkSampler Texture::CreateSampler(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t mipLevels)
	{
		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		createInfo.addressModeV = createInfo.addressModeU;
		createInfo.addressModeW = createInfo.addressModeU;


		VkPhysicalDeviceProperties pdp = { };
		vkGetPhysicalDeviceProperties(p_device, &pdp);

		createInfo.maxAnisotropy = pdp.limits.maxSamplerAnisotropy / 2.f;
		createInfo.anisotropyEnable = VK_TRUE;

		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		createInfo.unnormalizedCoordinates = VK_FALSE;

		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_NEVER;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.minLod = 0.f;
		createInfo.maxLod = static_cast<float>(mipLevels);
		createInfo.mipLodBias = 0.f; //optional...

		VkSampler nTextureSampler;
		VK_CHECK_RESULT(vkCreateSampler(l_device, &createInfo, nullptr, &nTextureSampler));

		return nTextureSampler;
	}

	Texture::~Texture() 
	{
		if (cLogicalDevice != VK_NULL_HANDLE) 
		{
			vkDestroySampler(cLogicalDevice, mSampler, nullptr);
			vkDestroyImageView(cLogicalDevice, mImageView, nullptr);
			vkDestroyImage(cLogicalDevice, mImage, nullptr);
			vkFreeMemory(cLogicalDevice, mMemory, nullptr);
		}
	}

	Texture::Texture(vk::Device* devicePtr, const std::string& fileName)
	{

		assert(devicePtr);
		//Might want to make command pool a member variable.

		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = fileName == "" ? nullptr : stbi_load((TEXTURE_PATH + fileName).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		if (!pixels)
		{
			std::cerr << "could not load in specified texture " + std::string(TEXTURE_PATH + fileName) << std::endl;
			//TODO: generate checker-board texture for objects.
			return;

		}

		uint64_t bytePerPixel = 4;
		VkDeviceSize imageSize = (uint64_t)textureWidth * (uint64_t)textureHeight * bytePerPixel; //4 bytes per pixel.

		/*uint32_t mipLevels = vk::util::CalculateMipLevels(textureWidth, textureHeight); -- commented out because I don't understand it yet. */ 
		uint32_t mipLevels = 1;

		vk::Buffer stagingBuffer = vk::Buffer(devicePtr->physical, devicePtr->logical,
			static_cast<size_t>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pixels);

		this->mImage = vk::init::CreateImage(devicePtr->physical,
			devicePtr->logical, textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->mMemory);

		VkCommandPool tempCmdPool = vk::init::CommandPool(devicePtr->logical, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

		vk::util::TransitionImageLayout(devicePtr->logical, tempCmdPool, devicePtr->graphicsQueue.handle, this->mImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		/*vk::util::GenerateMipMaps(p_device, l_device, cmdPool, gfxQueue, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, (uint32_t)textureWidth, (uint32_t)textureHeight, mipLevels);*/
		
		vk::util::copyBufferToImage(devicePtr->logical, tempCmdPool,
			stagingBuffer.handle, 
			devicePtr->graphicsQueue.handle,
			this->mImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight)); //copy contents of the image (stored in buffer) into the image.

		//transition the image layout to shader read only for sampling in the shader.
		vk::util::TransitionImageLayout(devicePtr->logical, tempCmdPool, devicePtr->graphicsQueue.handle, this->mImage, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

		//end of creating mipmaps.;

		vkDestroyCommandPool(devicePtr->logical, tempCmdPool, nullptr);
		stagingBuffer.Destroy();
		stbi_image_free(pixels);

		this->mImageView = CreateImageView(devicePtr->logical, this->mImage, mipLevels);

		this->mSampler = CreateSampler(devicePtr->physical, devicePtr->logical, mipLevels);

		this->descriptor = { 
			this->mSampler, 
			this->mImageView, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
		};	

		this->cLogicalDevice = devicePtr->logical;
	}


	static void InitializeTexture(Texture& texture, 
		vk::Device* devicePtr,
		const std::string& fileName = "", 
		unsigned char* data = nullptr, 
		VkExtent2D dimensions = {0,0})
	{
		assert(devicePtr != nullptr);
		//Might want to make command pool a member variable.

		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = nullptr;

		if (data == nullptr)
		{
			pixels = fileName == "" ? nullptr : stbi_load((TEXTURE_PATH + fileName).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

			if (!pixels)
			{
				std::cerr << "could not load in specified texture " + std::string(TEXTURE_PATH + fileName) << std::endl;
				//TODO: generate checker-board texture for objects.
				return;

			}
		}
		else
		{
			pixels = data;
			textureWidth = dimensions.width;
			textureHeight = dimensions.height;
		}

		uint64_t bytePerPixel = 4;
		VkDeviceSize imageSize = (uint64_t)textureWidth * (uint64_t)textureHeight * bytePerPixel; //4 bytes per pixel.

		/*uint32_t mipLevels = vk::util::CalculateMipLevels(textureWidth, textureHeight); -- commented out because I don't understand it yet. */
		uint32_t mipLevels = 1;

		vk::Buffer stagingBuffer = vk::Buffer(devicePtr->physical, devicePtr->logical,
			static_cast<size_t>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)pixels);

		texture.mImage = vk::init::CreateImage(devicePtr->physical,
			devicePtr->logical, textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.mMemory);

		VkCommandPool tempCmdPool = vk::init::CommandPool(devicePtr->logical, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

		vk::util::TransitionImageLayout(devicePtr->logical, tempCmdPool, devicePtr->graphicsQueue.handle, texture.mImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		/*vk::util::GenerateMipMaps(p_device, l_device, cmdPool, gfxQueue, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, (uint32_t)textureWidth, (uint32_t)textureHeight, mipLevels);*/

		vk::util::copyBufferToImage(devicePtr->logical, tempCmdPool,
			stagingBuffer.handle,
			devicePtr->graphicsQueue.handle,
			texture.mImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight)); //copy contents of the image (stored in buffer) into the image.

		//transition the image layout to shader read only for sampling in the shader.
		vk::util::TransitionImageLayout(devicePtr->logical, tempCmdPool, devicePtr->graphicsQueue.handle, texture.mImage, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

		//end of creating mipmaps.;

		vkDestroyCommandPool(devicePtr->logical, tempCmdPool, nullptr);
		stagingBuffer.Destroy();
		stbi_image_free(pixels);

		texture.mImageView = Texture::CreateImageView(devicePtr->logical, texture.mImage, mipLevels);

		texture.mSampler = Texture::CreateSampler(devicePtr->physical, devicePtr->logical, mipLevels);

		texture.descriptor.sampler = texture.mSampler;
		texture.descriptor.imageView = texture.mImageView;
		texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		texture.cLogicalDevice = devicePtr->logical;


	}


	Texture::Texture(vk::Device* devicePtr, const fastgltf::Asset& asset, fastgltf::Image& gltfImage)
	{
		int width;
		int height;
		int nChannels;

		std::visit(fastgltf::visitor
		{
			[](auto& arg) {},
			[&](fastgltf::sources::URI& path)
			{
				assert(path.fileByteOffset == 0); // We don't support offsets with stbi.
				assert(path.uri.isLocalPath()); // We're only capable of loading local files.
				
				const std::string filePath(path.uri.path().begin(), path.uri.path().end());

				/**this = Texture(graphicsContextInfo, filePath);*/
				InitializeTexture(*this, devicePtr, filePath);

			},
			[&](fastgltf::sources::Vector& vector)
			{
				//load from raw data, probably want a more generic function that takes in data ptr instead of just filename.
				unsigned char* data = stbi_load_from_memory(reinterpret_cast<uint8_t*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()),
				&width, &height, &nChannels, 4);

				if (data) {
					VkExtent2D dimensions = { (uint32_t)width, (uint32_t)height };
					/**this = Texture(graphicsContextInfo, "", data, dimensions);*/
					InitializeTexture(*this, devicePtr, "", data, dimensions);
				}

			},
			[&](fastgltf::sources::BufferView& view)
			{
				const fastgltf::BufferView& bufferView = asset.bufferViews[view.bufferViewIndex];
				const fastgltf::Buffer& buffer = asset.buffers[bufferView.bufferIndex];

				std::visit(fastgltf::visitor{

					[](auto& arg) {},
					[&](fastgltf::sources::Vector& vector) {
						unsigned char* data = stbi_load_from_memory(reinterpret_cast<uint8_t*>(vector.bytes.data() + bufferView.byteOffset), 
						static_cast<int>(bufferView.byteLength),
						&width, &height, &nChannels, 4);

						if (data) {
							VkExtent2D dimensions = { (uint32_t)width, (uint32_t)height };
							/**this = Texture(graphicsContextInfo, "", data, dimensions);*/
							InitializeTexture(*this, devicePtr, "", data, dimensions);
						}
					}

				}, buffer.data);

			},
		}, gltfImage.data);
	}

}