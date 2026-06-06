#ifndef VK_CUBEMAP_HPP
#define VK_CUBEMAP_HPP

//NOTE: entire implementation is WIP
#include "vkTexture.h"

namespace vk
{
    class Cubemap : public Texture
    {
    public:
        Cubemap() = default;
        Cubemap( const vk::Device* devicePtr,  const vk::TextureCreateInfo& createInfo );
        ~Cubemap() override = default;
    private:
        const char* CUBEMAP_DIR = "art/extern-textures/cubemaps/";
    };


}

#endif