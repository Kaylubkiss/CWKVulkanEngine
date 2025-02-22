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

class Application
{
private:	
	vk::Window mWindow;
	bool guiWindowIsFocused = false;

public:
	void run();
	~Application();

	const Time& GetTime();
	void RequestExit();
	vk::Window& GetWindow();
	bool WindowisFocused();
	void ToggleObjectVisibility(SDL_Keycode keysym,uint8_t lshift);

	//void SelectWorldObjects(const int& mouseX, const int& mouseY);

	Camera& GetCamera();
	void UpdateUniformViewMatrix();

private:

	Time mTime;
	Camera mCamera;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkCommandBuffer secondaryCmdBuffer = VK_NULL_HANDLE;

	vk::TextureManager mTextureManager;
	vk::ObjectManager mObjectManager;

	PhysicsSystem mPhysics;
	vk::GraphicsSystem mGraphicsSystem;

	bool exitApplication = false;

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	void CreateWindow(vk::Window& appWindow);
	void CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow);

	void InitPhysicsWorld();

	void DrawGui(VkCommandBuffer cmdBuffer);

	bool init();
	void loop();
	void exit();

	void InitGui();
	void CleanUpGui();

};





