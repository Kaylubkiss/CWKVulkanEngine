#pragma once
#include "Common.h"
#include "Mesh.h"
#include <SDL2/SDL.h>




class Application
{
public:
	const VkPhysicalDevice& PhysicalDevice();
	const VkDevice& LogicalDevice();
	const VkQueue& GraphicsQueue();
	const VkCommandPool& CommandPool();

	void run();
private:
	uint64_t timeNow;
	uint64_t timeBefore;

	double deltaTime;
	const float timeStep = 1.f / 60.f;

	Object debugCube;
	Object debugCube2;

	PhysicsCommon mPhysicsCommon;
	PhysicsWorld* mPhysicsWorld = nullptr;



	VkDebugUtilsMessengerEXT debugMessenger;

	ImGui_ImplVulkanH_Window guiWindowData;

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

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSets = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; //dunno if this should be here...

	std::vector<Buffer> uniformBuffers;

	VkImage textureImage;
	VkDeviceMemory textureMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkFormat depthFormat;

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
	void FillDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	/*void CreateDebugMessenger();*/
	void CreateWindow();
	void CreateWindowSurface();
	void EnumeratePhysicalDevices();
	void FindQueueFamilies();
	void CreateLogicalDevice();
	void CreateRenderPass();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateFrameBuffers();
	void CreateDescriptorSets();
	void WriteDescriptorSets();
	void CreatePipeline(VkPipelineShaderStageCreateInfo* pStages, int numStages);
	void CreateCommandPools();
	void CreateCommandBuffers();
	void CreateSemaphores();
	void CreateFences();
	void CreateBuffers();
	void CreateUniformBuffers();
	VkPipelineShaderStageCreateInfo CreateShaderModule(const char* name, VkShaderModule& shaderModule, VkShaderStageFlagBits stage);
	void RecreateSwapChain();
	void ResizeViewport();
	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels,
					 VkFormat format, VkImageTiling tiling, 
					 VkImageUsageFlags usage, VkMemoryPropertyFlags flags, 
					 VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLayerCount
					);

	void CreateDepthResources();
	
	void GenerateMipMaps(VkImage image, VkFormat imgFormat, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels);

	void CreateTexture(const std::string& fileName);
	void CreateTextureView(const VkImage& textureImage, uint32_t mipLevels);
	void CreateTextureSampler(uint32_t mipLevels);

	void InitPhysics();

	bool UpdateInput();

	void DrawGui();

	void CreateCubeMap();

	VkFormat findSupportedFormat(const std::vector<VkFormat>& possibleFormats, VkImageTiling tiling, VkFormatFeatureFlags features);


	bool init();
	void loop();
	void exit();

	void ComputeDeltaTime();
	void Render();

	void InitGui();
	void CleanUpGui();
};





