#pragma once
#include "vkInit.h"
#include <vulkan/vulkan.h>

namespace vk
{
	namespace debug 
	{
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			{
				//suggested to put '\n' here because std::endl is slower: it writes newline and flushes stream.
				std::cerr << pCallbackData->pMessage << '\n' << '\n';
			}

			return VK_FALSE;
		}


		static void BindMessengerToInstance(const VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger)
		{
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::init::DebugMessengerCreateInfo();

			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

			if (func != nullptr) {

				func(instance, &debugCreateInfo, nullptr, &debugMessenger);
			}
			else
			{
				throw std::runtime_error("could not set up debug messenger");
			}

		}
	}
}
