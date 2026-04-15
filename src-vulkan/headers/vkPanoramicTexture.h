#pragma once
#include "vkTexture.h"

namespace vk
{
    class PanoramicTexture : public Texture //technically a type of cubemap, but inheriting cubemap would be useless.
    {
    public:
        void Create( const vk::Device* devicePtr, const std::vector<std::string>& fileNames, std::mutex& transferMutex ) override
        {
            int width, height, nChannels;
            float* pixels = stbi_loadf(fileNames[0].c_str(), &width, &height, &nChannels, 0);

            if (pixels == nullptr)
            {
                throw std::runtime_error("PanoramicTexture::Create() failed!\n");
            }


            std::cout << "\033[32m" << "successfully loaded Panormaic Texture in PanoramicTexture::Create()... " << "\033[0m\n";

            stbi_image_free(pixels);
        }
    };


}