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
	bool exitApplication = false;

public:
	void run();
	Application() = default;
	~Application();

	const Timer& GetTime();
	void RequestExit();

	void SelectWorldObjects(const vk::Window& appWindow,
							Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);

	PhysicsSystem& GetPhysics();
	vk::TextureManager& TextureManager();
	vk::ObjectManager& ObjectManager();

	vk::ContextBase* Context();

private:

	Timer mTime;
	PhysicsSystem mPhysics;

	vk::TextureManager mTextureManager;
	vk::ObjectManager objectManager;

	std::unique_ptr<vk::ContextBase> graphicsContext;

	void init();
	void loop();
	void exit();

};





