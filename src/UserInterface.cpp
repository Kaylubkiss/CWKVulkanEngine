#include "UserInterface.h"
#include "renderer/vkInit.h"
#include "imgui.h"

void UserInterface::Init( const UserInterfaceInitInfo& initInfo )
{
	assert(initInfo.contextLogicalDevice != VK_NULL_HANDLE);

	if ( isInitialized )
	{
		return;
	}

	c_device = initInfo.contextLogicalDevice;

	InitializeUIDescriptorPool();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

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
	init_info.DescriptorPool = UIDescriptorPool;
	init_info.PipelineInfoMain.RenderPass = initInfo.renderPass;
	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.MinImageCount = 2;
	init_info.ImageCount = initInfo.minImages; //TODO: we assume that there is a backbuffer to render into.
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = vk::util::check_vk_result;

	ImGui_ImplVulkan_Init(&init_info);

	isInitialized = true;
	isToggled = true;

}

void UserInterface::Destroy()
{
	if ( isInitialized )
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		vkDestroyDescriptorPool(c_device, UIDescriptorPool, nullptr);

		isInitialized = false;
	}
}

bool UserInterface::WantsEvents()
{
	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void UserInterface::InitializeUIDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		vk::init::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gMaxFramesInFlight * max_textures)
	};

	VkDescriptorPoolCreateInfo descriptorPoolCI =
		vk::init::DescriptorPoolCreateInfo(poolSizes, gMaxFramesInFlight * max_textures); //2 for swapchain image count.

	VK_CHECK_RESULT(vkCreateDescriptorPool(c_device, &descriptorPoolCI, nullptr, &UIDescriptorPool));

}

void UserInterface::CheckBox( const std::string& label, bool* condition )
{
	ImGui::Checkbox(label.c_str(), condition);
}

void UserInterface::Slider( const std::string& label, glm::vec3& position, float min, float max )
{
	float* data[3] = { &position.x, &position.y, &position.z };
	ImGui::SliderFloat3(label.c_str(), *data, min, max);
}

void UserInterface::SeparatorText( const std::string& text )
{
	ImGui::SeparatorText(text.c_str());
}

void UserInterface::ComboBox()
{
	//ImGui::BeginMenuBar(,);

	//ImGui::Combo

}

bool UserInterface::CollapsingHeader( const std::string& label )
{
	return ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
}

void UserInterface::Prepare()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void UserInterface::Render( VkCommandBuffer cmdBuffer )
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}

bool UserInterface::IsToggled()
{
	return isInitialized && isToggled;
}
void UserInterface::Toggle( bool x )
{
	isToggled = x;
}

