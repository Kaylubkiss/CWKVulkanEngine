#pragma once
#include "vkPanoramicTexture.h"

namespace test
{
    inline void LoadPanoramicImage(vk::Device* devicePtr, std::vector<std::string>& fileNames, std::mutex& loadMutex)
    {
        vk::PanoramicTexture panoramicTexture;

        vk::TextureCreateInfo texture_create_info = { fileNames[0], VK_FORMAT_R32G32B32A32_SFLOAT };
        panoramicTexture = vk::PanoramicTexture(devicePtr, {texture_create_info }, loadMutex);
    }
}