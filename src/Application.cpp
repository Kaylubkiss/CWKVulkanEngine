#include "Application.h"
#include <iostream>
#include "vkDebug.h"
#include "vkInit.h"
#include "Controller.h"
#include "Physics.h"
#include "vkFreddyHeadContext.h"
#include "vkShadowMapContext.h"
#include "vkDeferredShadingContext.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>


//NOTE: to remove pesky warnings from visual studio, on dynamically allocated arrays,
//I've used the syntax: *(array + i) to access the array instead of array[i].
//the static analyzer of visual studio is bad.


Camera& Application::GetCamera()
{
	return this->mCamera;
}

PhysicsSystem& Application::GetPhysics() 
{
	return this->mPhysics;
}

vk::TextureManager& Application::TextureManager() 
{
	return this->mTextureManager;
}

vk::ContextBase* Application::Context() {

	return graphicsContext.get();
}

void Application::run() 
{
	//initialize all resources.
	Application::init();
	 
	//render, update, render, update...
	Application::loop();

	//cleanup resources
	Application::exit();
}


void Application::init() 
{
	this->graphicsContext = std::make_unique<vk::DeferredContext>();

	vk::DeferredContext* freddyScene = static_cast<vk::DeferredContext*>(graphicsContext.get());
	this->mCamera = Camera({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f } , { 0,1,0 });

	this->mTextureManager.Init(this->graphicsContext.get());

	this->mObjectManager.Init(
		&this->mTextureManager, 
		graphicsContext.get()->PhysicalDevice(), 
		graphicsContext.get()->LogicalDevice()
	);
	
	graphicsContext->InitializeScene(mObjectManager);
	
	mTime = Timer(SDL_GetPerformanceCounter());
}


const Timer& Application::GetTime()
{
	return this->mTime;
}


void Application::SelectWorldObjects(const vk::Window& appWindow, 
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

}

void Application::RequestExit() 
{
	this->exitApplication = true;
}


void Application::loop()
{
	//render graphics.
	while (exitApplication == false)
	{	
		double dt = mTime.CalculateDeltaTime();

		Controller::MoveCamera(mCamera, dt);

		mPhysics.Update(dt);

		this->mObjectManager.Update(mPhysics.InterpFactor());

		graphicsContext->RecordCommandBuffers(this->mObjectManager);

		//sync this up with primary command buffer in graphics system...
		graphicsContext->Render();
	}
	
	//when we're done with the loop, we should make sure the logical device is flushed.
	graphicsContext->WaitForDevice();
}


void Application::exit()
{
	mTextureManager.Destroy(graphicsContext->LogicalDevice());
	mObjectManager.Destroy(graphicsContext->LogicalDevice());
}


Application::~Application()
{
}







