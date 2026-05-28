#pragma once

//NOTE: entire implementation is WIP
#include "vkTexture.h"

namespace vk
{
    class Cubemap : public Texture
    {
    public:
        Cubemap() = default;
        Cubemap( vk::Device* devicePtr,  const std::vector<vk::TextureCreateInfo>& createInfos, std::mutex& transferMutex );
        ~Cubemap() override = default;
    private:
        const char* CUBEMAP_DIR = "art/extern-textures/cubemaps/";
    };


}