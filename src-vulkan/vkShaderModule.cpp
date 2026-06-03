#include "vkShaderModule.h"
#include "vkInit.h"
#include <sys/stat.h>

namespace vk
{
    ShaderModuleInfo::ShaderModuleInfo( const VkDevice l_device, std::string_view filename,
        VkShaderStageFlagBits shaderFlags )
    {
        m_shaderStageFlags = shaderFlags;
        c_device = l_device;

        switch (shaderFlags)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:
                m_shaderKind = shaderc_vertex_shader;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                m_shaderKind = shaderc_fragment_shader;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                m_shaderKind = shaderc_geometry_shader;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                m_shaderKind = shaderc_compute_shader;
                break;
            default:
                throw std::runtime_error("Unknown shader kind");
        }

        std::string sourceFilePath = SHADER_PATH + std::string(filename);
        auto spirvPath = vk::spirv::ReadSourceAndWriteToSpirv(sourceFilePath, m_shaderKind);

        if (spirvPath.has_value() == false)
        {
            return;
        }

        m_filePath = std::string(filename);

        m_handle = vk::init::ShaderModule(l_device, spirvPath.value().c_str());

        struct stat fileStat = {};
        if (stat(sourceFilePath.c_str(), &fileStat) != 0)
        {
            std::cerr << "[ERROR] Can't open file: " << m_filePath << '\n';
        }
        else
        {
            m_modificationTime = fileStat.st_mtime;
        }
    }

    ShaderModuleInfo::~ShaderModuleInfo()
    {
        if ( c_device != VK_NULL_HANDLE )
        {
            vkDestroyShaderModule(c_device, m_handle, nullptr);
        }
    }

    void ShaderModuleInfo::SetModificationTime( time_t timeStamp )
    {
        m_modificationTime = timeStamp;
    }

    VkShaderModule ShaderModuleInfo::GetHandle() const
    {
        return m_handle;
    }

    VkShaderStageFlagBits ShaderModuleInfo::GetShaderStageFlags() const
    {
        return m_shaderStageFlags;
    }

    const std::string& ShaderModuleInfo::GetFileName() const
    {
        return m_filePath;
    }

    time_t ShaderModuleInfo::GetModificationTime() const
    {
        return m_modificationTime;
    }

    shaderc_shader_kind ShaderModuleInfo::GetShaderKind() const
    {
        return m_shaderKind;
    }

    ShaderModuleInfo& ShaderModuleInfo::operator=( ShaderModuleInfo&& other ) noexcept
    {

        if (this != &other)
        {
            std::swap(this->m_filePath, other.m_filePath);
            std::swap(this->m_shaderKind, other.m_shaderKind);
            std::swap(this->m_modificationTime, other.m_modificationTime);
            std::swap(this->m_shaderStageFlags, other.m_shaderStageFlags);
            std::swap(this->m_handle, other.m_handle);
            std::swap(this->c_device, other.c_device);
        }

        return *this;
    }

    ShaderModuleInfo::ShaderModuleInfo( ShaderModuleInfo&& other ) noexcept
    {
        this->m_filePath = other.m_filePath;
        this->m_shaderKind = other.m_shaderKind;
        this->m_modificationTime = other.m_modificationTime;
        this->m_shaderStageFlags = other.m_shaderStageFlags;
        this->m_handle = other.m_handle;
        this->c_device = other.c_device;

        other.c_device = VK_NULL_HANDLE;
    }
}