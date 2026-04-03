#include "vkShaderModule.h"
#include <sys/stat.h>

#define SHADER_PATH "shaders/"

namespace vk
{
    ShaderModuleInfo::ShaderModuleInfo( const VkDevice l_device, std::string_view filename,
        VkShaderStageFlagBits shaderFlags )
    {
        mFlags = shaderFlags;

        switch (shaderFlags)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:
                mShaderKind = shaderc_vertex_shader;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                mShaderKind = shaderc_fragment_shader;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                mShaderKind = shaderc_geometry_shader;
                break;
            default:
                throw std::runtime_error("Unknown shader kind");
        }

        std::string sourceFilePath = SHADER_PATH + std::string(filename);
        auto spirvPath = vk::spirv::ReadSourceAndWriteToSpirv(sourceFilePath, mShaderKind);

        if (spirvPath.has_value() == false)
        {
            return;
        }
        else
        {
            mFilePath = sourceFilePath;
        }

        mHandle = vk::init::ShaderModule(l_device, spirvPath.value().c_str());

        struct stat fileStat = {};
        if (stat(mFilePath.c_str(), &fileStat) != 0)
        {
            std::cerr << "[ERROR] Can't open file: " << mFilePath << '\n';
        }
        else
        {
            lastModificationTime = fileStat.st_mtime;
        }
    }
}