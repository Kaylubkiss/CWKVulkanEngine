#pragma once
#include "SpirvHelper.h"

//ShaderModuleInfo
namespace vk
{
    struct ShaderModuleInfo
    {
        VkShaderModule mHandle = VK_NULL_HANDLE;
        time_t lastModificationTime = 0;
        std::string mFilePath = "";

        VkShaderStageFlagBits mFlags = VK_SHADER_STAGE_ALL;
        shaderc_shader_kind mShaderKind = shaderc_glsl_infer_from_source; /*arguments in runtime shader compilation */

        /*
            *@brief intializer list for a ShaderModuleInfo object. Compiles the specified shader source to sprv and also sets the shader stage flags.

            *@param l_device: logical device associated with the application's vulkan instance.
            *@param filename: the name of the shader source.
            *@param shaderFlags: specifies what shader stage the source file is working in.
            *@param shaderc_kind: similar to shaderFlags, argument needed for shader compilation to sprv.
        */
        ShaderModuleInfo() = default;
        ShaderModuleInfo( const VkDevice l_device, std::string_view filename, VkShaderStageFlagBits shaderFlags );
    };
}