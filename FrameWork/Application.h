#pragma once
#include "Common.h"
#include "Time.h"
#include "Physics.h"
#include "Camera.h"
#include "Object.h"
#include "Debug.h"
#include "Controller.h"
#include <SDL2/SDL.h>

struct uTransformObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};


class Application
{
public:
	unsigned long long width = 640;
	unsigned long long height = 480;
	bool guiWindowIsFocused = false;

	const VkPhysicalDevice& PhysicalDevice();
	const VkDevice& LogicalDevice();
	const VkQueue& GraphicsQueue();
	const VkCommandPool& CommandPool();
	const VkPipeline& GetTrianglePipeline();
	const VkPipeline& GetLinePipeline();
	int GetTexture(const char* fileName);
	VkPipelineLayout* GetPipelineLayout();
	const std::vector<Texture>& Textures();
	const Time& GetTime();
	Physics& PhysicsSystem();
	void RequestExit();
	SDL_Window* GetWindow() const;
	bool WindowisFocused();
	void ToggleObjectVisibility(SDL_Keycode keysym,uint8_t lshift);
	void SelectWorldObjects(const int& mouseX, const int& mouseY);
	Camera& GetCamera();
	void UpdateUniformViewMatrix();

	void run();
	~Application();
private:

	uTransformObject uTransform =
	{
		glm::mat4(1.), //model
		//glm::lookAt(glm::vec3(0.f, 0.f , 10.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f,1.f,0.f)), //view
		glm::mat4(1.f),
		glm::perspective(glm::radians(45.f), (float)width / height,  0.1f, 1000.f) //proj
	};

	Time mTime;

	Object debugCube;
	Object debugCube2;
	Object debugCube3;
	Camera mCamera;

	Physics mPhysics;

	Controller mController;

	bool exitApplication = false;

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

	std::vector<VkPipelineLayout> pipelineLayouts;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipeline linePipeline = VK_NULL_HANDLE;

	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

	VkFence inFlightFence = VK_NULL_HANDLE;

	VkViewport m_viewPort = {};
	VkRect2D m_scissor = {};

	VkImage* swapChainImages = nullptr;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; //dunno if this should be here...

	std::vector<Buffer> uniformBuffers;

	std::vector<Texture> mTextures;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkFormat depthFormat;

	const char* enabledLayerNames[1] = 
	{
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
	void CreateWindow();

	void CreateWindowSurface();
	void EnumeratePhysicalDevices();
	void FindQueueFamilies();
	void CreateLogicalDevice();
	void CreateRenderPass();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateFrameBuffers();
	//void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSets();
	void WriteDescriptorSets();
	void CreatePipelineLayout();
	void CreatePipeline(VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology, VkPipeline& pipelineHandle);
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
	void CreateTextureView(const VkImage& textureImage, VkImageView& textureImageView, uint32_t mipLevels);
	void CreateTextureSampler(VkSampler& textureSampler, uint32_t mipLevels);

	void InitPhysicsWorld();

	bool CheckValidationSupport();

	bool UpdateInput();

	void DrawGui();

	void CreateCubeMap();

	VkFormat findSupportedFormat(const std::vector<VkFormat>& possibleFormats, VkImageTiling tiling, VkFormatFeatureFlags features);


	bool init();
	void loop();
	void exit();

	void Render();

	void InitGui();
	void CleanUpGui();

	void DestroyObjects();
};





