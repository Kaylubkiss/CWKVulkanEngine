#include "UserInterface.h"
#include "vkInit.h"

namespace vk {

	UserInterface::UserInterface(const UserInterfaceInitInfo& initInfo)
	{
		this->contextLogicalDevice = initInfo.contextLogicalDevice;
		UserInterface::InitializeUIDescriptorPool();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

		if (!ImGui_ImplSDL2_InitForVulkan(initInfo.contextWindow)) {

			throw std::runtime_error("couldn't initialize imgui with vulkan\n");
		}

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = initInfo.contextInstance;
		init_info.PhysicalDevice = initInfo.contextPhysicalDevice;
		init_info.Device = initInfo.contextLogicalDevice;
		init_info.QueueFamily = initInfo.contextQueue.family;
		init_info.Queue = initInfo.contextQueue.handle;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = this->UIDescriptorPool;
		init_info.RenderPass = initInfo.renderPass;
		init_info.Subpass = 0;
		init_info.MinImageCount = 2;
		init_info.ImageCount = 2; //TODO: we assume that there is a backbuffer to render into.
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = vk::util::check_vk_result;
		ImGui_ImplVulkan_Init(&init_info);

	}

	void UserInterface::Destroy() 
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		vkDestroyDescriptorPool(contextLogicalDevice, this->UIDescriptorPool, nullptr);
	}

	void UserInterface::InitializeUIDescriptorPool() 
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1) 
		};

		VkDescriptorPoolCreateInfo descriptorPoolCI = vk::init::DescriptorPoolCreateInfo(poolSizes, 2); //2 for swapchain image count. 

		VK_CHECK_RESULT(vkCreateDescriptorPool(this->contextLogicalDevice, &descriptorPoolCI, nullptr, &UIDescriptorPool));

	}

}