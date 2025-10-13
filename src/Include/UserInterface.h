#pragma once

#include "vkTexture.h"

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

		uint32_t minImages = 0;
	};

	class UserInterface {
		public:
			UserInterface() = default;
			UserInterface(const UserInterfaceInitInfo& initInfo);
			~UserInterface() = default;
			void Destroy();

			void Prepare();
			void Render(VkCommandBuffer cmdBuffer); //after main rendering

			//types of options
			void CheckBox(std::string label, bool* condition);
			void Slider(std::string label, glm::vec3& position, float min = -100, float max = 100);
			void SeparatorText(std::string text);
			void ComboBox();
			void DisplayImages();
			bool CollapsingHeader(std::string label);

			void AddImage(const vk::Texture& texture);
		private:
			void InitializeUIDescriptorPool();
			VkDevice contextLogicalDevice = VK_NULL_HANDLE;
			VkDescriptorPool UIDescriptorPool = VK_NULL_HANDLE; //just for the sampler.
			uint32_t max_textures = 100;
			std::vector<VkDescriptorSet> displayTextures;
	};

}