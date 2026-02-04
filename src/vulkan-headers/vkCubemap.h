#pragma once

//NOTE: entire implementation is WIP
#include <stb_image.h> //THIS SHOULD NOT BE SPECIFIED HERE BUT THERE ARE SYMBOL ERRS

constexpr const char* CUBEMAP_DIR = "assets/textures/cubemaps/";

namespace vk
{
    class Cubemap
    {
    public:
        Cubemap() = default;
        ~Cubemap() = default;

        //just one particular environment map for now
        static void CreateImage( vk::Device* devicePtr )
        {
            constexpr size_t image_count = 6;
            //texture sizes should be square and/or the same in a cubemap
            int image_width, image_height, channels;

            std::array<stbi_uc*, image_count> texture_data = {};

            std::array<std::string, image_count> file_names = {
                "IceRiver/negx.jpg", //left
                "IceRiver/posx.jpg", //right
                "IceRiver/negy.jpg", //up
                "IceRiver/posy.jpg", //down
                "IceRiver/posz.jpg", //forward
                "IceRiver/negz.jpg" //back
            };

            for (size_t i = 0; i < image_count; ++i) {
                texture_data[i] = stbi_load((CUBEMAP_DIR + file_names[i]).c_str(),
                    &image_width, &image_height, &channels, STBI_rgb_alpha);
            }

            const VkDeviceSize image_size = image_width * image_height *
                channels * static_cast<int>(image_count);
            const VkDeviceSize layer_size = image_size / static_cast<VkDeviceSize>(image_count);


            vk::Buffer stagingBuffer = vk::Buffer(devicePtr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, image_size);
            stagingBuffer.Map();

            //copy the layers
            /*for (size_t i = 0; i < image_count; ++i)
            {
                memcpy(static_cast<char*>(stagingBuffer.GetMappedMemory()) + (layer_size * i), texture_data[i], layer_size);
            }
            */

            //INSTEAD: use copyBuffertoImage()


            //deallocate resources
            for (size_t i = 0; i < image_count; ++i)
            {
                stbi_image_free(texture_data[i]);
            }
            stagingBuffer.Destroy();

            std::cout << "\033[32m" << "successfully loaded Cubemap in CreateImage()... " << "\033[0m\n";
        }
    private:



    };


}