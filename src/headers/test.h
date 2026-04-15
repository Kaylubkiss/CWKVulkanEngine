#pragma once
#include "vkPanoramicTexture.h"

namespace test
{
    inline void LoadPanoramicImage(vk::Device* devicePtr, std::vector<std::string>& fileNames, std::mutex& loadMutex)
    {
        vk::PanoramicTexture panoramicTexture;

        panoramicTexture.Create(devicePtr, fileNames, loadMutex);
    }
}