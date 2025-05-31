#pragma once
#include "Timer.h"
#include "Camera.h"
#include "Debug.h"
#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkWindow.h"
#include "vkGraphicsSystem.h"
#include "TextureManager.h"
#include "Physics.h"
#include "HotReloader.h"

class Application
{
private:	
	vk::Window mWindow;
	bool guiWindowIsFocused = false;

public:
	void run();
	Application() = default;
	~Application();

	const Timer& GetTime();
	void RequestExit();
	vk::Window& GetWindow();
	bool WindowisFocused(); 

	void SelectWorldObjects(const vk::Window& appWindow,
							Camera& camera, const vk::uTransformObject& uTransform, PhysicsSystem& physics);

	Camera& GetCamera();
	PhysicsSystem& GetPhysics();

private:

	Timer mTime;
	Camera mCamera;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkCommandBuffer secondaryCmdBuffer = VK_NULL_HANDLE;

	vk::TextureManager mTextureManager;
	vk::ObjectManager mObjectManager;

	PhysicsSystem mPhysics;
	vk::HotReloader mHotReloader;

	//POINTER
	vk::GraphicsSystem* mGraphicsSystem = nullptr;

	bool exitApplication = false;

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	void CreateWindow(vk::Window& appWindow);
	void CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow);

	void DrawGui(VkCommandBuffer cmdBuffer);

	bool init();
	void loop();
	void exit();

	void InitGui();
	void CleanUpGui();

};





