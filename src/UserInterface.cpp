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
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange ;

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
		init_info.ImageCount = initInfo.minImages; //TODO: we assume that there is a backbuffer to render into.
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = vk::util::check_vk_result;
		ImGui_ImplVulkan_Init(&init_info);

		isInitialized = true;

	}

	void UserInterface::Destroy() 
	{
		if (isInitialized) 
		{
			for (auto texture : displayTextures)
			{
				ImGui_ImplVulkan_RemoveTexture(texture);
			}

			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();

			vkDestroyDescriptorPool(contextLogicalDevice, this->UIDescriptorPool, nullptr);
		}
	}

	void UserInterface::InitializeUIDescriptorPool() 
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gMaxFramesInFlight * max_textures) 
		};

		VkDescriptorPoolCreateInfo descriptorPoolCI = vk::init::DescriptorPoolCreateInfo(poolSizes, gMaxFramesInFlight * max_textures); //2 for swapchain image count. 

		VK_CHECK_RESULT(vkCreateDescriptorPool(this->contextLogicalDevice, &descriptorPoolCI, nullptr, &UIDescriptorPool));

	}

	void UserInterface::CheckBox(std::string label, bool* condition)
	{
		ImGui::Checkbox(label.c_str(), condition);
	}

	void UserInterface::Slider(std::string label, glm::vec3& position, float min, float max) 
	{	
		float* data[3] = { &position.x, &position.y, &position.z };
		ImGui::SliderFloat3(label.c_str(), *data, min, max);
	}

	void UserInterface::SeparatorText(std::string text)
	{
		ImGui::SeparatorText(text.c_str());
	}

	void UserInterface::ComboBox() 
	{
		//ImGui::BeginMenuBar(,);

		//ImGui::Combo

	}

	void UserInterface::DisplayImages() 
	{
		for (auto texture : displayTextures) {

			ImGui::Image(texture, ImVec2(64, 64));
			ImGui::SameLine();
		}

	}

	bool UserInterface::CollapsingHeader(std::string label)
	{
		return ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
	}

	void UserInterface::AddImage(const vk::Texture& texture) 
	{
		std::cout << "adding image to UI\n";

		if (texture.mTextureImageView == VK_NULL_HANDLE)
		{
			std::cout << "huh\n" << std::endl;
		}

		displayTextures.emplace_back(
			ImGui_ImplVulkan_AddTexture(
				texture.mTextureSampler,
				texture.mTextureImageView,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			)
		);
	}

	void UserInterface::Prepare() 
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}

	void UserInterface::Render(VkCommandBuffer cmdBuffer) 
	{
		//ImGui::ShowDemoWindow();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	}



}