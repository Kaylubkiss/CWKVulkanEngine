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
	bool exitApplication = false;

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
	PhysicsSystem mPhysics;
	vk::HotReloader mHotReloader;

	VkInstance mInstance = VK_NULL_HANDLE;

	vk::TextureManager mTextureManager;
	vk::ObjectManager mObjectManager;
	vk::GraphicsSystem mGraphicsSystem;

	void CreateWindow(vk::Window& appWindow);
	void CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow);

	void DrawGui(VkCommandBuffer cmdBuffer);

	bool init();
	void loop();
	void exit();

	void InitGui();
	void CleanUpGui();

};





