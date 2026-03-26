#pragma once

namespace vk
{
    class Instance
    {
    public:
        Instance() = default;
        ~Instance();

        VkInstance GetHandle() const;

        void Create( std::vector<const char*>& extensions, std::vector<const char*>& layers );\
    private:
        VkInstance m_handle = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    };

}


