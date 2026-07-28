#ifndef USER_INTERFACE_HPP
#define USER_INTERFACE_HPP

#include "vkTexture.h"

struct UserInterfaceInitInfo
{
	VkInstance contextInstance = VK_NULL_HANDLE;
	VkDevice contextLogicalDevice = VK_NULL_HANDLE;
	VkPhysicalDevice contextPhysicalDevice = VK_NULL_HANDLE;
	SDL_Window* contextWindow = nullptr;

	vk::Queue contextQueue = {};
	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkExtent2D viewPortExtent = {};

	uint32_t minImages = 0;
};

class UserInterface
{
public:
	static void Init( const UserInterfaceInitInfo& initInfo );
	static void Destroy();

	static bool WantsEvents();

	static void Prepare();
	static void Render( VkCommandBuffer cmdBuffer ); //after main rendering

	template<typename T>
	static void TextData( const char* fmt, T value )
	{
		ImGui::Text(fmt, value);
	}

	static void CheckBox( const std::string& label, bool* condition );
	static void Slider( const std::string& label, glm::vec3& position, float min = -100, float max = 100 );
	static void SeparatorText( const std::string& text );
	static void ComboBox();
	static bool CollapsingHeader( const std::string& label );
private:
	static void InitializeUIDescriptorPool();
private:
	inline static VkDevice c_device = VK_NULL_HANDLE;
	inline static VkDescriptorPool UIDescriptorPool = VK_NULL_HANDLE; //just for the sampler.
	static constexpr uint32_t max_textures = 100;
	inline static bool isInitialized = false;
};

#endif