#ifndef VK_SHADER_MODULE_HPP
#define VK_SHADER_MODULE_HPP

#include "../SpirvHelper.h"

constexpr inline const char* SHADER_PATH = "shaders/";

//ShaderModuleInfo
namespace vk
{
    class ShaderModuleInfo
    {
    public:
        /*
            *@brief intializer list for a ShaderModuleInfo object. Compiles the specified shader source to sprv and also sets the shader stage flags.

            *@param l_device: logical device associated with the application's vulkan instance.
            *@param filename: the name of the shader source.
            *@param shaderFlags: specifies what shader stage the source file is working in.
            *@param shaderc_kind: similar to shaderFlags, argument needed for shader compilation to sprv.
        */
        ShaderModuleInfo() = default;
        ShaderModuleInfo( const VkDevice l_device, std::string_view filename, VkShaderStageFlagBits shaderFlags );

        ShaderModuleInfo& operator=( const ShaderModuleInfo& other ) = delete;
        ShaderModuleInfo( const ShaderModuleInfo& other ) = delete;

        ShaderModuleInfo& operator=( ShaderModuleInfo&& other ) noexcept;
        ShaderModuleInfo( ShaderModuleInfo&& other ) noexcept;

        ~ShaderModuleInfo();

        void SetModificationTime( time_t timeStamp );

        [[nodiscard]] VkShaderModule GetHandle() const;
        [[nodiscard]] VkShaderStageFlagBits GetShaderStageFlags() const;
        [[nodiscard]] const std::string& GetFileName() const;
        [[nodiscard]] time_t GetModificationTime() const;
        [[nodiscard]] shaderc_shader_kind GetShaderKind() const;

    private:
        std::string m_filePath;
        time_t m_modificationTime = 0;

        VkShaderModule m_handle = VK_NULL_HANDLE;

        VkShaderStageFlagBits m_shaderStageFlags = VK_SHADER_STAGE_ALL;
        shaderc_shader_kind m_shaderKind = shaderc_glsl_infer_from_source; /*arguments in runtime shader compilation */

        VkDevice c_device = VK_NULL_HANDLE;
    };
}

#endif