#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Timer.h"
#include "Physics.h"
#include "Camera.h"
#include "vkRendererBase.h"
#include "SceneManager.h"
#include "../../src-vulkan/headers/vkTextureManager.h"
#include "../../src-vulkan/headers/vkDescriptorManager.h"

class Application
{
public:
	Application();
	~Application();

	PhysicsSystem& GetPhysics();
	Timer& GetTimer();
	vk::RendererBase* GetVulkanRenderer() const;

	void run();

	void RequestExit();
	/*void SelectWorldObjects(const vk::Window& appWindow,
		Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);*/
private:
	Timer mTime;
	PhysicsSystem m_physics;
	std::unique_ptr<vk::RendererBase> m_vulkanGraphicsContext;
	AssetManager m_assetManager;
	SceneManager m_sceneManager;
	vk::TextureManager m_textureManager;
	vk::DescriptorManager m_descriptorManager;

	bool exitApplication = false;
};

#endif





