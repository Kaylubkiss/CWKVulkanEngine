#pragma once
#include "vkPanoramicTexture.h"

namespace test
{
    inline void LoadPanoramicImage(vk::Device* devicePtr, std::vector<std::string>& fileNames, std::mutex& loadMutex)
    {
        vk::PanoramicTexture panoramicTexture;

        vk::TextureCreateInfo texture_create_info = { fileNames[0], VK_FORMAT_R16G16B16A16_SFLOAT };
        panoramicTexture.Create(devicePtr, {texture_create_info }, loadMutex);
    }
}