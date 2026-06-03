#pragma once

namespace vk
{
    class Instance
    {
    public:
        Instance() = default;
        Instance( std::vector<const char*>& extensions, std::vector<const char*>& layers );

        Instance( const Instance& other ) = delete;
        Instance& operator=( const Instance& other ) = delete;

        Instance( Instance&& other ) noexcept;
        Instance& operator=( Instance&& other ) noexcept;

        ~Instance();

        VkInstance GetHandle() const;
        VkPhysicalDevice GetGPU( VkPhysicalDeviceType type );
    private:
        VkInstance m_handle = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    };

}


