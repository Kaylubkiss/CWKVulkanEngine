#pragma once


#include <vulkan/vulkan.h>
#include "vkTexture.h"
#include <vector>

namespace vk 
{
	struct TextureManager 
	{
	public:
		void Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const std::string& fileName);
		void Deallocate(const VkDevice l_device);
	private:
		
		//I know that string comparison is slow. Will work on that later.
		std::vector<Texture> mTextures;
	};

}