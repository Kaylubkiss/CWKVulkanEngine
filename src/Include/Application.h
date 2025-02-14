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



class Application
{
private:	
	vk::Window mWindow;
	bool guiWindowIsFocused = false;

public:
	void run();

	const VkPipeline& GetLinePipeline();
	const Time& GetTime();
	void RequestExit();
	vk::Window& GetWindow() const;
	bool WindowisFocused();
	void ToggleObjectVisibility(SDL_Keycode keysym,uint8_t lshift);

	//void SelectWorldObjects(const int& mouseX, const int& mouseY);

	Camera& GetCamera() const;
	void UpdateUniformViewMatrix();

private:
	~Application();

	Time mTime;
	Camera mCamera;

	VkInstance m_instance = VK_NULL_HANDLE;

	Controller mController;
	
	ObjectManager mObjectManager;

	PhysicsSystem mPhysics;

	vk::GraphicsSystem mGraphicsSystem;

	bool exitApplication = false;

	VkDebugUtilsMessengerEXT debugMessenger;

	std::vector<VkPipelineLayout> pipelineLayouts;

	VkPipeline linePipeline = VK_NULL_HANDLE;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; //dunno if this should be here...

	LightInfoObject mLights;

	void CreateWindow(vk::Window& appWindow);
	void CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow);
	void CreateDescriptorSets();
	void WriteDescriptorSets();
	void CreateFences(const VkDevice l_device);

	void InitPhysicsWorld();


	void DrawGui(VkCommandBuffer cmdBuffer);


	bool init();
	void loop();
	void exit();

	void InitGui();
	void CleanUpGui();

};





