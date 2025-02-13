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
#include "vkTexture.h"
#include "vkGraphicsSystem.h"


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
	const std::vector<vk::Texture>& Textures();
	const Time& GetTime();
	Physics& PhysicsSystem();
	void RequestExit();
	SDL_Window* GetWindow() const;
	bool WindowisFocused();
	void ToggleObjectVisibility(SDL_Keycode keysym,uint8_t lshift);
	//void SelectWorldObjects(const int& mouseX, const int& mouseY);
	Camera& GetCamera();
	void UpdateUniformViewMatrix();

	void run();
private:
	~Application();

	VkInstance m_instance = VK_NULL_HANDLE;

	uTransformObject uTransform = {};

	vk::DepthResources depthResources;

	vk::GraphicsSystem graphicsSystem;

	Time mTime;

	ObjectManager mObjectManager;

	Camera mCamera;

	Physics mPhysics = Physics();
	  
	Controller mController;

	bool exitApplication = false;

	VkDebugUtilsMessengerEXT debugMessenger;

	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	VkFramebuffer* frameBuffer = nullptr;
	
	/*VkImageView* imageViews = nullptr;*/

	VkShaderModule shaderVertModule = VK_NULL_HANDLE;
	VkShaderModule shaderFragModule = VK_NULL_HANDLE;

	std::vector<VkPipelineLayout> pipelineLayouts;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipeline linePipeline = VK_NULL_HANDLE;

	//VkCommandPool commandPool = VK_NULL_HANDLE;
	//VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	//VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
	//VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

	//VkFence inFlightFence = VK_NULL_HANDLE;


	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; //dunno if this should be here...

	std::vector<vk::Buffer> uniformBuffers;

	LightInfoObject mLights;

	void CreateWindow(vk::Window& appWindow);
	void CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow);

	/*void CreateImageViews(const VkDevice l_device, VkImage* images, uint32_t imageCount);*/

	void CreateFrameBuffers(const VkDevice l_device, VkImageView* imageViews, VkImageView depthImageView, uint32_t imageCount, const VkViewport& vp);
	void CreateDescriptorSets();
	void WriteDescriptorSets();
	void CreatePipelineLayout(const VkDevice l_device, const VkDescriptorSetLayout descriptorSetLayout);
	void CreatePipeline(VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology, VkPipeline& pipelineHandle);
	VkCommandBuffer CreateCommandBuffer(const VkDevice l_device, const VkCommandPool cmdPool);
	void CreateSemaphores();
	void CreateFences(const VkDevice l_device);
	void CreateUniformBuffers(const VkPhysicalDevice p_device, const VkDevice l_device);
	void RecreateSwapChain();
	void ResizeViewport(VkViewport& vp, SDL_Window* windowHandle);
	 
	void InitPhysicsWorld();


	void DrawGui(VkCommandBuffer cmdBuffer);


	bool init();
	void loop();
	void exit();

	void Render(const VkDevice l_device);

	void InitGui();
	void CleanUpGui();

	void DestroyObjects();
};





