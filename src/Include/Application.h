#pragma once
#include "Timer.h"
#include "Camera.h"
#include "Physics.h"
#include "Object.h"
#include "TextureManager.h"
#include "ObjectManager.h"
#include "vkContextBase.h"
#include "GLTFLoading.h"

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
	void ResizeWindow();

	void SelectWorldObjects(const vk::Window& appWindow,
							Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);

	PhysicsSystem& GetPhysics();
	vk::TextureManager& TextureManager();
	vk::ObjectManager& ObjectManager();

	vk::ContextBase* Context();

private:

	//this MUST be declared at the top so that it's destructor is called last.
	std::unique_ptr<vk::ContextBase> graphicsContext;

	Timer mTime;
	PhysicsSystem mPhysics;

	vk::TextureManager mTextureManager;
	vk::ObjectManager objectManager;

	void init();
	void loop();
	void exit();

};





