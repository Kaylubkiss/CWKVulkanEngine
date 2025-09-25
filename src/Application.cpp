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

void Application::InitGui() 
{
	//IMGUI_CHECKVERSION();
	//ImGui::CreateContext();

	//ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	//// Setup Platform/Renderer backends
	//if (!ImGui_ImplSDL2_InitForVulkan(this->mWindow.sdl_ptr)) {

	//	throw std::runtime_error("couldn't initialize imgui for vulkan!!!\n");
	//	return;
	//}


	//ImGui_ImplVulkan_InitInfo init_info = {};
	//init_info.Instance = this->m_instance;
	//init_info.PhysicalDevice = mGraphicsSystem.PhysicalDevice();
	//init_info.Device = mGraphicsSystem.LogicalDevice();
	//init_info.QueueFamily = mGraphicsSystem.GraphicsQueue().family;
	//init_info.Queue = mGraphicsSystem.GraphicsQueue().handle;
	//init_info.PipelineCache = VK_NULL_HANDLE;
	//init_info.DescriptorPool = mGraphicsU;
	//init_info.RenderPass = this->m_renderPass;
	//init_info.Subpass = 0;
	//init_info.MinImageCount = 2;
	//init_info.ImageCount = this->imageCount;
	//init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	//init_info.Allocator = nullptr;
	//init_info.CheckVkResultFn = vk::util::check_vk_result;
	//ImGui_ImplVulkan_Init(&init_info);
}



void Application::init() 
{
	this->graphicsContext = std::make_unique<vk::ShadowMapScene>();

	vk::ShadowMapScene* freddyScene = static_cast<vk::ShadowMapScene*>(graphicsContext.get());
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

void Application::DrawGui(VkCommandBuffer cmdBuffer)
{

	//ImGui_ImplVulkan_NewFrame();
	//ImGui_ImplSDL2_NewFrame();
	//ImGui::NewFrame();

	//const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	//ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + main_viewport->WorkSize.x / 15, main_viewport->WorkPos.y + main_viewport->WorkSize.y / 10), ImGuiCond_Once);
	//ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x / 3, main_viewport->WorkSize.y / 2), ImGuiCond_Once);


	//ImGuiWindowFlags window_flags = 0;
	//window_flags |= ImGuiWindowFlags_MenuBar;
	//// Main body of the Demo window starts here.
	//if (!ImGui::Begin("Asset Log", nullptr, window_flags))
	//{
	//	// Early out if the window is collapsed, as an optimization
	//	this->guiWindowIsFocused = ImGui::IsWindowFocused();
	//	ImGui::End();
	//	ImGui::Render();
	//	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	//	return;
	//}

	//this->guiWindowIsFocused = ImGui::IsWindowFocused();
	//
	//ImGui::End();
	//ImGui::Render();
	//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);


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


void Application::CleanUpGui() 
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}





