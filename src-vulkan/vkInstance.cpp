#include "vkInstance.h"

namespace vk
{

	VkInstance Instance::GetHandle() const
	{
		return m_handle;
	}

	VkPhysicalDevice Instance::GetGPU( VkPhysicalDeviceType type )
	{
		std::vector<VkPhysicalDevice> gpus;
		std::optional<size_t> g_index;

		//list the physical devices
		uint32_t max_devices = 0;

		//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(m_handle, &max_devices, nullptr));

		if (max_devices == 0)
		{
			std::cerr << "could not find any GPUs to use!\n";
			throw std::runtime_error("Device::FindPhysicalDevices() Failed!\n");
		}

		gpus.resize( max_devices );

		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(m_handle, &max_devices, gpus.data()));

		for (size_t i = 0; i < max_devices; ++i)
		{

			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;

			vkGetPhysicalDeviceProperties(gpus[i], &properties);
			vkGetPhysicalDeviceFeatures(gpus[i], &features);

			if ((properties.deviceType == type) &&
				features.geometryShader && features.samplerAnisotropy)
			{
				std::cout << "picked device " << i << '\n';

				g_index = i;
				break;
			}
		}

		if (g_index.has_value() == false)
		{
			std::cerr << "could not find suitable physical device!";
			throw std::runtime_error("Device::FindPhysicalDevices() Failed!\n");
		}

		VkPhysicalDevice gpu = gpus[g_index.value()];

		return gpu;
	}

    Instance::Instance( std::vector<const char*>& extensions, std::vector<const char*>& layers )
    {
		//create instance info.
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_4;
		appInfo.pApplicationName = "Caleb's Vulkan Engine";
		appInfo.engineVersion = 1;
		appInfo.pNext = nullptr;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.flags = 0;

		createInfo.pApplicationInfo = &appInfo;

		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::util::DebugMessengerCreateInfo();
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debugCreateInfo);

		if (!vk::util::CheckInstanceExtensionSupport(extensions.data(),
			static_cast<int>(extensions.size())))
		{
			throw std::runtime_error("one or more instance extensions are not supported\n");
		}

		if (!vk::util::CheckInstanceLayerSupport(layers.data(),
			static_cast<int>(layers.size())))
		{
			throw std::runtime_error("one or more layers are not supported\n");
		}

		createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		createInfo.ppEnabledLayerNames = layers.data();

		VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_handle));

		if (layers.size() > 0)
		{
			//create messenger object handle that actually calls debugCallback.
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_handle,
				"vkCreateDebugUtilsMessengerEXT");

				if (func != nullptr)
				{
					VK_CHECK_RESULT(func(m_handle, &debugCreateInfo, nullptr, &m_debugMessenger));
				}
				else
				{
					throw std::runtime_error("failed to load vkCreateDebugUtilsMessengerEXT");
				}
		}
    }

	Instance::Instance( Instance&& other ) noexcept
	{
		this->m_handle = other.m_handle;
		this->m_debugMessenger = other.m_debugMessenger;

		other.m_handle = VK_NULL_HANDLE;
	}

	Instance& Instance::operator=( Instance&& other ) noexcept
	{
		if (this != &other)
		{
			std::swap(this->m_handle, other.m_handle);
			std::swap(this->m_debugMessenger, other.m_debugMessenger);
		}

		return *this;
	}

    Instance::~Instance()
    {
		if (m_handle != VK_NULL_HANDLE)
		{
			if (m_debugMessenger != nullptr)
			{
				auto deleteFunction = (PFN_vkDestroyDebugUtilsMessengerEXT)
					(vkGetInstanceProcAddr(m_handle, "vkDestroyDebugUtilsMessengerEXT"));
				if (deleteFunction != nullptr)
				{
					deleteFunction(m_handle, m_debugMessenger, nullptr);
				}
			}

			vkDestroyInstance(m_handle, nullptr);
			m_handle = VK_NULL_HANDLE;
		}
    }
}