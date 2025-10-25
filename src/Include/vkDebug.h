#pragma once
#include "vkInit.h"
#include "vkUtility.h"

namespace vk
{
	namespace debug 
	{
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)	
		{
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT || 
				messageType >= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			{
				//suggested to put '\n' here because std::endl is slower: it writes newline and flushes stream.
				std::cout << pCallbackData->pMessage << '\n' << '\n';
			}

			return VK_FALSE;
		}

		static VKAPI_ATTR VkBool32 debugReportCallback(
			VkDebugReportFlagsEXT                       flags,
			VkDebugReportObjectTypeEXT                  objectType,
			uint64_t                                    object,
			size_t                                      location,
			int32_t                                     messageCode,
			const char* pLayerPrefix,
			const char* pMessage,
			void* pUserData) 
		{
			std::cout << pMessage << '\n' << '\n';

			return VK_FALSE;
		}

		static void SetInstanceDebugObject(VkInstance instance, VkDevice l_device, VkDebugUtilsObjectNameInfoEXT&  instanceDebug)
		{
			auto SetDebugUtilsObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));

			instanceDebug = {};
			instanceDebug.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			instanceDebug.objectHandle = *reinterpret_cast<uint64_t*>(&instance);
			instanceDebug.objectType = VK_OBJECT_TYPE_INSTANCE;
			instanceDebug.pObjectName = "vulkan instance";

			VK_CHECK_RESULT(SetDebugUtilsObjectName(l_device, &instanceDebug))
		}

		static void CreateDebugReportCallback(VkInstance instance, VkDebugReportCallbackEXT& debugReportObj)
		{
			auto CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));

			VkDebugReportCallbackCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			createInfo.pfnCallback = debugReportCallback;
			createInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
							   VK_DEBUG_REPORT_WARNING_BIT_EXT |
							   VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
							   VK_DEBUG_REPORT_ERROR_BIT_EXT |
							   VK_DEBUG_REPORT_DEBUG_BIT_EXT;


			VK_CHECK_RESULT(CreateDebugReportCallback(instance, &createInfo, nullptr, &debugReportObj))

		}
	}
}
