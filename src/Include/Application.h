#pragma once
#include "Common.h"
#include "Timer.h"
#include "Camera.h"
#include "Debug.h"
#include "Controller.h"
#include "BlinnPhong.h"
#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkWindow.h"


struct uTransformObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class Application
{
private:	
	vk::Window mWindow;

public:
	bool guiWindowIsFocused = false;

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

	uTransformObject uTransform = {};

	vk::DepthResources depthResources;

	Time mTime;

	ObjectManager mObjectManager;

	Camera mCamera;

	Physics mPhysics = Physics();
	  
	Controller mController;

	bool exitApplication = false;

	VkDebugUtilsMessengerEXT debugMessenger;

	//variabless
	VkInstance m_instance = VK_NULL_HANDLE;

	//VkDevice m_logicalDevice = VK_NULL_HANDLE;

	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	VkFramebuffer* frameBuffer = nullptr;
	
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



	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; //dunno if this should be here...

	std::vector<Buffer> uniformBuffers;

	std::vector<Texture> mTextures;


	LightInfoObject mLights;

	/*VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkImage* swapChainImages = nullptr;
	uint32_t imageCount = -1;*/

	VkSurfaceCapabilitiesKHR deviceCapabilities = {};

	//
	void CreateWindow(vk::Window& appWindow);
	void CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow);

	void CreateSwapChain();
	void CreateImageViews();
	void CreateFrameBuffers();
	void CreateDescriptorSets();
	void WriteDescriptorSets();
	void CreatePipelineLayout();
	void CreatePipeline(VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology, VkPipeline& pipelineHandle);
	void CreateCommandPools(const VkDevice& l_device);
	void CreateCommandBuffers();
	void CreateSemaphores();
	void CreateFences();
	void CreateUniformBuffers();
	void RecreateSwapChain();
	void ResizeViewport(VkViewport& vp, SDL_Window* windowHandle);
	
	void GenerateMipMaps(VkImage image, VkFormat imgFormat, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels);

	void CreateTexture(const std::string& fileName);
	void CreateTextureView(const VkImage& textureImage, VkImageView& textureImageView, uint32_t mipLevels);
	void CreateTextureSampler(VkSampler& textureSampler, uint32_t mipLevels);

	void InitPhysicsWorld();


	void DrawGui();


	bool init();
	void loop();
	void exit();

	void Render();

	void InitGui();
	void CleanUpGui();

	void DestroyObjects();
};





