#pragma once
#include "vkTexture.h"

namespace vk
{
    class PanoramicTexture : public Texture //technically a type of cubemap, but inheriting cubemap would be useless.
    {
    public:
        void Create( const vk::Device* devicePtr, const std::vector<std::string>& fileNames, std::mutex& transferMutex ) override;
    };


}