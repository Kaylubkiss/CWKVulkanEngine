//
// Created by cwkmi on 7/28/2026.
//

#ifndef CKVULKAN_VKCONTEXT_HPP
#define CKVULKAN_VKCONTEXT_HPP

#include "vkInstance.h"
#include "vkDevice.h"

namespace vk
{
    struct Context
    {
        vk::Instance instance;
        vk::Device device;
    };
}
#endif //CKVULKAN_VKCONTEXT_HPP