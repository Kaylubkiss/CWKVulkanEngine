#pragma once

#include "Common.h"

namespace vk 
{
	struct UserInterfaceInitInfo 
	{
		VkInstance contextInstance = VK_NULL_HANDLE;
		VkDevice contextLogicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice contextPhysicalDevice = VK_NULL_HANDLE;

		SDL_Window* contextWindow = nullptr;

		vk::Queue contextQueue = {};
		VkRenderPass renderPass = VK_NULL_HANDLE;
	};

	class UserInterface {
		public:
			UserInterface() = default;
			UserInterface(const UserInterfaceInitInfo& initInfo);
			~UserInterface() = default;
			void Destroy();

			void RenderUI(VkCommandBuffer cmdBuffer); //after main rendering

		private:
			void InitializeUIDescriptorPool();
			VkDevice contextLogicalDevice = VK_NULL_HANDLE;
			VkDescriptorPool UIDescriptorPool = VK_NULL_HANDLE; //just for the sampler.

	};

}