#pragma once
#include <vulkan/vulkan.h> //in A: drive under "VulkanSDK"
#include <SDL2/SDL.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <vector>



class Application
{
public:
	void run();
private:
	//variabless
	SDL_Window* window = nullptr;
	VkInstance m_instance = VK_NULL_HANDLE;
	
	VkPhysicalDevice* m_physicalDevices = nullptr;
	unsigned int device_index = -1;

	VkDevice m_logicalDevice = VK_NULL_HANDLE;

	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkSurfaceKHR m_windowSurface = nullptr;
	VkFramebuffer* frameBuffer = nullptr;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkImageView* imageViews = nullptr;

	VkShaderModule shaderVertModule = VK_NULL_HANDLE;
	VkShaderModule shaderFragModule = VK_NULL_HANDLE;
	VkShaderModule* ShaderStages[2] = { &shaderVertModule, &shaderFragModule };

	VkBuffer vkBuffer = VK_NULL_HANDLE;

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;

	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

	VkFence inFlightFence = VK_NULL_HANDLE;

	VkViewport m_viewPort = {};
	VkRect2D m_scissor = {};

	VkImage* swapChainImages = nullptr;

	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout; //dunno if this should be here...

	std::vector<void*> uniformBufferMemory;

	const char* enabledLayerNames[1] = {
		"VK_LAYER_KHRONOS_validation"
	};

	const char* deviceExtensions[1] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	uint32_t graphicsFamily = -1; //tutorial used std::optional, which basically specifies if a value has been found or not.
	uint32_t presentFamily = -1;
	uint32_t imageCount = -1;

	VkSurfaceCapabilitiesKHR deviceCapabilities = {};

	VkQueue graphicsQueue = {};
	VkQueue presentQueue = {};

	//functions
	void CreateInstance();
	void CreateWindow();
	void CreateWindowSurface();
	void EnumeratePhysicalDevices();
	void FindQueueFamilies();
	void CreateLogicalDevice();
	void CreateRenderPass();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateFrameBuffers();
	VkPipelineShaderStageCreateInfo CreateShaderModule(const char* name, VkShaderModule& shaderModule, VkShaderStageFlagBits stage);
	void CreateBuffers();
	void CreateDescriptorSets();
	void CreatePipeline(VkPipelineShaderStageCreateInfo* pStages, int numStages);
	void CreateCommandPools();
	void CreateCommandBuffers();
	void CreateSemaphores();
	void CreateFences();
	void CreateUniformBuffers();
	void RecreateSwapChain();


	bool init();
	void loop();
	void exit();
};