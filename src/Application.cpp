#include "Input.h"
#include "vkDeferredRenderer.h"

static void Locatecwd()
{
	std::filesystem::path cwd = std::filesystem::current_path();
	while (cwd.has_parent_path() && cwd.parent_path() != cwd)
	{
		if (std::filesystem::exists(cwd.string() + "/src/"))
		{
			std::filesystem::current_path(cwd/"src");
			return;
		}
		cwd = cwd.parent_path();
	}
}

Application::Application()
{
	Locatecwd();

	m_vulkanGraphicsContext = std::make_unique<vk::DeferredRenderer>(&m_textureManager, m_descriptorManager);

	m_assetManager.Init(m_vulkanGraphicsContext->GetDevicePtr(), &m_textureManager, 2);

	m_sceneManager.Init(m_assetManager);
}

PhysicsSystem& Application::GetPhysics() 
{
	return this->m_physics;
}

Timer& Application::GetTimer()
{
	return this->mTime;
}

vk::RendererBase* Application::GetVulkanRenderer() const
{
	return m_vulkanGraphicsContext.get();
}

void Application::run()
{
	if (m_vulkanGraphicsContext != nullptr)
	{
		//render graphics.
		while (exitApplication == false)
		{
			double realFrameTime = mTime.CalculateDeltaTime();

			float physicsTime = m_physics.InterpFactor(static_cast<float>(realFrameTime));

			m_sceneManager.Update( physicsTime, realFrameTime );

			m_vulkanGraphicsContext->Render(  m_sceneManager.GetSceneView() );
		}

		//when we're done with the loop, we should make sure the logical device is flushed.
		m_vulkanGraphicsContext->WaitForDevice();
	}
}

/*void Application::SelectWorldObjects(const vk::Window& appWindow,
									 Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics)
{
	
	int mouseX = 0, mouseY = 0;

	if (SDL_GetRelativeMouseMode() == SDL_FALSE) 
	{
		SDL_GetMouseState(&mouseX, &mouseY);
	}
	else 
	{
		mouseX = appWindow.center_x;
		mouseY = appWindow.center_y;
	}

	glm::vec4 cursorWindowPos(mouseX, mouseY, 1, 1);

	glm::vec4 cursorScreenPos = {};

	//ndc
	cursorScreenPos.x = (2 * cursorWindowPos.x) / appWindow.viewport.width - 1;
	cursorScreenPos.y = (2 * cursorWindowPos.y) / appWindow.viewport.height - 1; //vulkan is upside down.
	cursorScreenPos.z = 1;
	cursorScreenPos.w = 1;

	////eye

	////world 
	glm::vec4 ray_world_far = glm::inverse(uTransform.proj * uTransform.view) * cursorScreenPos;

	ray_world_far /= ray_world_far.w;

	cursorScreenPos.z = 0;
	glm::vec4 ray_world_near = glm::inverse(uTransform.proj * uTransform.view) * cursorScreenPos;

	ray_world_near /= ray_world_near.w;

	//2. cast ray from the mouse position and in the direction forward from the mouse position

	reactphysics3d::Vector3 rayStart(ray_world_near.x, ray_world_near.y, ray_world_near.z);

	reactphysics3d::Vector3 rayEnd(ray_world_far.x, ray_world_far.y, ray_world_far.z);

	Ray ray(rayStart, rayEnd);

	RaycastInfo raycastInfo = {};

	RayCastObject callbackObject;

	physics.World()->raycast(ray, &callbackObject);

}*/

void Application::RequestExit() 
{
	this->exitApplication = true;
}

Application::~Application()
{
	m_assetManager.Destroy();
	m_textureManager.Destroy();
	m_descriptorManager.Destroy();

	SDL_Quit();
}

