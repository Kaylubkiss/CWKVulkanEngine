#pragma once

//NOTE: entire implementation is WIP
#include "vkTexture.h"

constexpr const char* CUBEMAP_DIR = "art/extern-textures/cubemaps/";

namespace vk
{
    class Cubemap : public Texture
    {
    public:
        Cubemap() = default;
        ~Cubemap() override = default;

        //just one particular environment map for now
        void Create( const vk::Device* devicePtr,  const std::vector<vk::TextureCreateInfo>& createInfos, std::mutex& transferMutex ) override;
    };


}