#pragma once
#include "Timer.h"
#include "Camera.h"
#include "Physics.h"
#include "vkContextBase.h"
#include "vkInstance.h"
#include "SceneManager.h"
#include "ResourceManager.h"


class Application
{
public:
	Application() = default;
	~Application();

	Timer& GetTimer();
	[[nodiscard]] vk::ContextBase* GetVulkanContext() const;

	void run();

	void RequestExit();

	/*void SelectWorldObjects(const vk::Window& appWindow,
		Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);*/
private:
	void init();
	void loop();
	void exit();
private:
	Timer mTime;
	std::unique_ptr<vk::ContextBase> m_renderer;
	DescriptorManager m_descriptorManager;
	ResourceManager m_resourceManager;
	SceneManager m_sceneManager;

	bool exitApplication = false;

};





