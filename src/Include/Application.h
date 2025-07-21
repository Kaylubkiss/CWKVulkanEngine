#pragma once
#include "Timer.h"
#include "Camera.h"
#include "Debug.h"
#include "ObjectManager.h"
#include "vkWindow.h"
#include "vkContextBase.h"
#include "TextureManager.h"
#include "Physics.h"

class Application
{
private:	
	bool guiWindowIsFocused = false;
	bool exitApplication = false;

public:
	void run();
	Application() = default;
	~Application();

	const Timer& GetTime();
	void RequestExit();

	void SelectWorldObjects(const vk::Window& appWindow,
							Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);

	Camera& GetCamera();
	PhysicsSystem& GetPhysics();

	vk::ContextBase* Context();

private:

	Timer mTime;
	Camera mCamera;
	PhysicsSystem mPhysics;

	vk::TextureManager mTextureManager;

	//TODO: move objectmanager into the graphics context...?
	vk::ObjectManager mObjectManager;
	std::unique_ptr<vk::ContextBase> graphicsContext;

	void DrawGui(VkCommandBuffer cmdBuffer);

	void init();
	void loop();
	void exit();

	void InitGui();
	void CleanUpGui();

};





