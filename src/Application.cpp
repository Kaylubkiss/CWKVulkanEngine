#include "Application.h"
#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "vkUtility.h"
#include "vkDebug.h"
#include "vkInit.h"
#include "vkResource.h"


//NOTE: to remove pesky warnings from visual studio, on dynamically allocated arrays,
//I've used the syntax: *(array + i) to access the array instead of array[i].
//the static analyzer of visual studio is bad.


static const std::string shaderPath{ "Shaders/" };

const static glm::vec4 X_BASIS = { 1,0,0,0 };
const static glm::vec4 Y_BASIS = { 0,1,0,0 };
const static glm::vec4 Z_BASIS = { 0,0,1,0 };
const static glm::vec4 W_BASIS = { 0,0,0,1 };

void Application::UpdateUniformViewMatrix() 
{
	if (mCamera.isUpdated()) 
	{
		uTransform.view = mCamera.LookAt();
		memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));
	}
}

Camera& Application::GetCamera()
{
	return this->mCamera;
}

void Application::run() 
{
	//initialize all resources.
	init();

	//render, update, render, update...
	loop();

	//cleanup resources
	exit();
}

void Application::ToggleObjectVisibility(SDL_Keycode keysym, uint8_t lshift) 
{
	/*debugCube3.debugDrawObject.ToggleVisibility(keysym, lshift);
	debugCube2.debugDrawObject.ToggleVisibility(keysym, lshift);*/

}

void Application::CreateWindow(vk::Window& appWindow)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}

	appWindow.sdl_ptr = SDL_CreateWindow("Caleb's Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
	int(appWindow.viewport.width), int(appWindow.viewport.height), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (appWindow.sdl_ptr == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
	}


	
}

bool Application::WindowisFocused() 
{
	if (this->mWindow.sdl_ptr == NULL) 
	{
		return false;
	}

	uint32_t flags = SDL_GetWindowFlags(this->mWindow.sdl_ptr);

	return ((flags & SDL_WINDOW_INPUT_FOCUS) != 0);

}

void Application::CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow) 
{
	if (SDL_Vulkan_CreateSurface(appWindow.sdl_ptr, vkInstance, &appWindow.surface) != SDL_TRUE)
	{
		throw std::runtime_error("could not create window surface!");
	}
}


void Application::CreateUniformBuffers(const VkPhysicalDevice p_device, const VkDevice l_device)
{
	this->uniformBuffers.push_back(vk::Buffer(p_device, l_device, sizeof(uTransformObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)&uTransform));
	
}



const VkPipeline& Application::GetTrianglePipeline()
{
	return this->pipeline;
}

const VkPipeline& Application::GetLinePipeline()
{
	return this->linePipeline;
}

void Application::CreateDescriptorSets()
{
	//create descriptor pool
	VkDescriptorPoolSize poolSize[2] = {};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 2; //max numbers of frames in flight.

	//we are concerned about the fragment stage, so we double the descriptor count here.
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 1 * 2; //max numbers of frames in flight times two to accomodate the gui.

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = (uint32_t)2;
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = mTextures.size() > 0? mTextures.size() * 2 : 1; //max numbers of frames in flight.

	VK_CHECK_RESULT(vkCreateDescriptorPool(this->m_logicalDevice, &poolInfo, nullptr, &this->descriptorPool));

	for (size_t i = 0; i < mTextures.size(); ++i) 
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.descriptorPool = this->descriptorPool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &this->descriptorSetLayout;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(this->m_logicalDevice, &descriptorAllocInfo, &this->mTextures[i].mDescriptor));

	}
}

void Application::WriteDescriptorSets() 
{
	VkDescriptorBufferInfo uTransformbufferInfo = {};
	uTransformbufferInfo.buffer = uniformBuffers[0].handle;
	uTransformbufferInfo.offset = 0;
	uTransformbufferInfo.range = sizeof(uTransformObject);

	VkDescriptorBufferInfo uLightInfoBufferInfo = {};
	uLightInfoBufferInfo.buffer = mLights.mBuffer.handle;
	uLightInfoBufferInfo.offset = 0;
	uLightInfoBufferInfo.range = sizeof(LightInfoObject) - sizeof(int) * LightCountIndex::MAX_IND_COUNT;

	VkDescriptorBufferInfo bufferInfo[2] = { uTransformbufferInfo, uLightInfoBufferInfo };
	VkDescriptorImageInfo imageInfo = {};

	for (size_t i = 0; i < this->mTextures.size(); ++i) 
	{
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = this->mTextures[i].mTextureImageView;
		imageInfo.sampler = this->mTextures[i].mTextureSampler;

		VkWriteDescriptorSet descriptorWrite[3] = {};
		descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[0].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[0].dstBinding = 0;
		descriptorWrite[0].dstArrayElement = 0;
		descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[0].descriptorCount = 1; //how many buffers
		descriptorWrite[0].pBufferInfo = &uTransformbufferInfo;
		descriptorWrite[0].pImageInfo = nullptr; // Optional
		descriptorWrite[0].pTexelBufferView = nullptr; // Optional

		descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[1].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[1].dstBinding = 1;
		descriptorWrite[1].dstArrayElement = 0;
		descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[1].descriptorCount = 1; //how many buffers
		descriptorWrite[1].pBufferInfo = &uLightInfoBufferInfo;
		descriptorWrite[1].pImageInfo = nullptr; // Optional
		descriptorWrite[1].pTexelBufferView = nullptr; // Optional

		descriptorWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[2].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[2].dstBinding = 2;
		descriptorWrite[2].dstArrayElement = 0;
		descriptorWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite[2].descriptorCount = 1; //how many images
		descriptorWrite[2].pImageInfo = &imageInfo;
		descriptorWrite[2].pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(this->m_logicalDevice, 3, descriptorWrite, 0, nullptr);
	}

}

void Application::CreateSemaphores() 
{
	this->imageAvailableSemaphore = vk::init::CreateSemaphore(this->m_logicalDevice);
	this->renderFinishedSemaphore = vk::init::CreateSemaphore(this->m_logicalDevice);
}

void Application::CreateFences(const VkDevice l_device) 
{
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //to prevent indefinite waiting on first frame.

	VK_CHECK_RESULT(vkCreateFence(l_device, &fenceInfo, nullptr, &this->inFlightFence));
}

void Application::RecreateSwapChain() 
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(this->m_logicalDevice));


	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_physicalDevices[device_index], this->m_windowSurface, &this->deviceCapabilities));

	this->mWindowExtents.width = deviceCapabilities.currentExtent.width;
	this->mWindowExtents.height = deviceCapabilities.currentExtent.height;

	for (unsigned i = 0; i < this->imageCount; ++i)
	{
		vkDestroyFramebuffer(this->m_logicalDevice, this->frameBuffer[i], nullptr);
	}

	
	vkDestroyImageView(this->m_logicalDevice, this->depthImageView, nullptr);
	vkDestroyImage(this->m_logicalDevice, this->depthImage, nullptr);
	
	delete[] this->frameBuffer;
	delete[] this->swapChainImages;

	vkDestroySwapchainKHR(this->m_logicalDevice, this->swapChain, nullptr);

	CreateSwapChain();

	CreateImageViews();

	CreateDepthResources();

	CreateFrameBuffers();
}


void Application::ResizeViewport()
{
	int nWidth, nHeight;

	SDL_GetWindowSizeInPixels(this->mWindow.sdl_ptr, &nWidth, &nHeight);
	this->mWindow.viewport.width = (float)nWidth;
	this->mWindow.viewport.height = (float)nHeight;

	uTransform.proj = glm::perspective(glm::radians(45.f), (float)this->mWindow.viewport.width / this->mWindow.viewport.height, 0.1f, 1000.f); //proj
	uTransform.proj[1][1] *= -1.f;

	memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));

}

void Application::InitPhysicsWorld() 
{
	this->mObjectManager["cube"].InitPhysics(ColliderType::CUBE);
	
	reactphysics3d::Material& db2Material = this->mObjectManager["cube"].mPhysicsComponent.collider->getMaterial();
	db2Material.setBounciness(0.f);
	db2Material.setMassDensity(10.f);
	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->updateMassPropertiesFromColliders();
	
	this->mObjectManager["base"].InitPhysics(ColliderType::CUBE, BodyType::STATIC);
	
	mCamera.InitPhysics(BodyType::STATIC);


	this->mObjectManager["cube"].SetLinesArrayOffset(12);

	//this->mPhysicsWorld->setIsDebugRenderingEnabled(true);
	this->mPhysics.GetPhysicsWorld()->setIsDebugRenderingEnabled(true);

	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->setIsDebugEnabled(true);
	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->setIsDebugEnabled(true);
	
	//the order they were added to the physics world
	reactphysics3d::DebugRenderer& debugRenderer = this->mPhysics.GetPhysicsWorld()->getDebugRenderer();
	debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);
	
}

void Application::InitGui() 
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	// Setup Platform/Renderer backends
	if (!ImGui_ImplSDL2_InitForVulkan(this->window)) {

		throw std::runtime_error("couldn't initialize imgui for vulkan!!!\n");
		return;
	}


	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = this->m_instance;
	init_info.PhysicalDevice = this->m_physicalDevices[device_index];
	init_info.Device = this->m_logicalDevice;
	init_info.QueueFamily = this->graphicsFamily;
	init_info.Queue = this->graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = this->descriptorPool;
	init_info.RenderPass = this->m_renderPass;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = this->imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = vk::util::check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);
}


bool Application::init() 
{
	this->mCamera = Camera({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f } , { 0,1,0 });

	this->mWindow.viewport.width = 640;
	this->mWindow.viewport.height = 480;

	this->uTransform = {
		glm::mat4(1.f), //model
		this->mCamera.LookAt(), //view
		glm::perspective(glm::radians(45.f), (float)this->mWindow.viewport.width / this->mWindow.viewport.height, 0.1f, 1000.f) //proj
	};


	this->uTransform.proj[1][1] *= -1.f;


	CreateWindow(this->mWindow);

	m_instance = vk::init::CreateInstance(this->mWindow.sdl_ptr);

	CreateWindowSurface(this->m_instance, this->mWindow);

	//setup the debug callbacks... (optional...)

	this->graphicsSystem = vk::GraphicsSystem(this->m_instance, this->mWindow.surface);
	
	//retrieve queue family properties 
	// --> group of queues that have identical capabilities and are able to run in parallel 
	//		--> could be arithmetic, passing shaders, stuff like that.

	glm::mat4 modelTransform = glm::mat4(5.f);
	modelTransform[3] = glm::vec4(1.f, 0, -20.f, 1);

	mObjectManager.LoadObject("freddy", "freddy.obj", false, modelTransform);

	//object 2
	modelTransform = glm::mat4(1.f);
	modelTransform[3] = glm::vec4(0, 20, -5.f, 1);

	mObjectManager.LoadObject("cube", "cube.obj", true, modelTransform);

	//object 3
	const float dbScale = 30.f;
	modelTransform = glm::mat4(dbScale);
	modelTransform[3] = { 0.f, -5.f, 0.f, 1 };

	mObjectManager.LoadObject("base", "base.obj", true, modelTransform);

	
	// If you want to draw a triangle:
	// - create renderpass object
	CreateSwapChain();

	CreateImageViews();

	std::string vertexShaderPath = shaderPath + "blinnvert.spv";
	VkPipelineShaderStageCreateInfo shaderVertStageInfo = vk::util::CreateShaderModule(this->m_logicalDevice, vertexShaderPath.data(), this->shaderVertModule, VK_SHADER_STAGE_VERTEX_BIT);

	std::string fragShaderPath = shaderPath + "blinnfrag.spv";
	VkPipelineShaderStageCreateInfo shaderFragModuleInfo = vk::util::CreateShaderModule(this->m_logicalDevice, fragShaderPath.data(), this->shaderFragModule, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineShaderStageCreateInfo shaderStages[] = { shaderVertStageInfo, shaderFragModuleInfo };

	////create layout 
	this->commandPool = vk::init::CreateCommandPool(this->m_logicalDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	this->commandBuffer = CreateCommandBuffer(this->m_logicalDevice, this->commandPool);
	
	CreateUniformBuffers();


	mLights.Create({ 0, 10, 0 }, { 0, -1, 0 });
	
	CreateDepthResources();

	//ERROR: didn't create descriptoryLayout!!!

	CreatePipelineLayout();
	
	CreateRenderPass();
	CreateFrameBuffers();
	
	CreatePipeline(shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, this->pipeline);
	CreatePipeline(shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, this->linePipeline);

	//commands


	CreateSemaphores();

	CreateFences();

	
	mTime = Time(SDL_GetPerformanceCounter());

	while (mObjectManager.mThreadWorkers.isBusy()) { //wait until the jobs are done... 
	}

	this->mObjectManager["freddy"].UpdateTexture("texture.jpg");
	this->mObjectManager["freddy"].UpdatePipelineLayout(&this->pipelineLayouts.back());

	this->mObjectManager["cube"].UpdateTexture("puppy1.bmp");
	this->mObjectManager["cube"].UpdatePipelineLayout(&this->pipelineLayouts.back());

	this->mObjectManager["base"].UpdateTexture("puppy1.bmp");
	this->mObjectManager["base"].UpdatePipelineLayout(&this->pipelineLayouts.back());


	CreateDescriptorSets();
	WriteDescriptorSets();


	InitGui();

	InitPhysicsWorld();

	//ERROR: vksetcmdviewport? scissor? you made a dynamic viewport!!!


	return true;


}

SDL_Window* Application::GetWindow() const
{
	return this->window;
}

const Time& Application::GetTime()
{
	return this->mTime;
}

class RayCastObject : public RaycastCallback {
public:
	virtual decimal notifyRaycastHit(const RaycastInfo& info)
	{
		// Display the world hit point coordinates
		std::cout << " Hit point : " <<
			info.worldPoint.x <<
			info.worldPoint.y <<
			info.worldPoint.z <<
			std::endl;

		// Return a fraction of 1.0 to gather all hits
		return decimal(-1.0);
	}
};


//void Application::SelectWorldObjects(const int& mouseX, const int& mouseY)
//{
//	
//
//	glm::vec4 cursorWindowPos(mouseX, mouseY, 1, 1);
//
//	glm::vec4 cursorScreenPos = {};
//
//	//ndc
//	cursorScreenPos.x = (2 * cursorWindowPos.x) / this->mWindowExtents.width - 1;
//	cursorScreenPos.y = 1 - (2 * cursorWindowPos.y) / this->mWindowExtents.height; //vulkan is upside down.
//	cursorScreenPos.z = -1;
//	cursorScreenPos.w = 1;
//
//	////eye
//
//	////world 
//	glm::vec4 ray_world = glm::inverse(uTransform.view * uTransform.proj) * cursorScreenPos;
//
//	ray_world /= ray_world.w;
//
//	//2. cast ray from the mouse position and in the direction forward from the mouse position
//
//	glm::vec3 CameraPos = mCamera.Position();
//
//	reactphysics3d::Vector3 rayStart(CameraPos.x, CameraPos.y, CameraPos.z);
//
//	reactphysics3d::Vector3 rayEnd(ray_world.x, ray_world.y, ray_world.z);
//
//	Ray ray(rayStart, rayEnd);
//
//	RaycastInfo raycastInfo = {};
//
//	RayCastObject callbackObject;
//
//	this->mPhysics.GetPhysicsWorld()->raycast(ray, &callbackObject);
//
//}

void Application::DrawGui(VkCommandBuffer cmdBuffer)
{

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + main_viewport->WorkSize.x / 15, main_viewport->WorkPos.y + main_viewport->WorkSize.y / 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x / 3, main_viewport->WorkSize.y / 2), ImGuiCond_Once);


	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_MenuBar;
	// Main body of the Demo window starts here.
	if (!ImGui::Begin("Asset Log", nullptr, window_flags))
	{
		// Early out if the window is collapsed, as an optimization
		this->guiWindowIsFocused = ImGui::IsWindowFocused();
		ImGui::End();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
		return;
	}

	this->guiWindowIsFocused = ImGui::IsWindowFocused();
	
	ImGui::End();
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);


}

void Application::Render(const VkDevice l_device) 
{
	
	VK_CHECK_RESULT(vkWaitForFences(this->m_logicalDevice, 1, &this->inFlightFence, VK_TRUE, UINT64_MAX))
	VK_CHECK_RESULT(vkResetFences(this->m_logicalDevice, 1, &this->inFlightFence))

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(this->m_logicalDevice, this->swapChain, UINT64_MAX, this->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);


	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		ResizeViewport();
		return;
	}

	assert(result == VK_SUCCESS);

	VK_CHECK_RESULT(vkResetCommandBuffer(this->commandBuffer, 0))


	////always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
	//always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	//everything else is default...

	//resetting command buffer should be implicit with reset flag.
	VK_CHECK_RESULT(vkBeginCommandBuffer(this->commandBuffer, &cmdBufferBeginInfo))

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = this->m_renderPass;
	renderPassInfo.framebuffer = this->frameBuffer[imageIndex];
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = deviceCapabilities.currentExtent;

	VkClearValue clearColors[2] = {};
	clearColors[0].color = { {0.f, 0.f, 0.f, 1.f} };
	clearColors[1].depthStencil = { 1.f, 0 };

	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearColors;

	//put this in a draw frame
	vkCmdBeginRenderPass(this->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//bind the graphics pipeline
	vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

	/*vkCmdSetViewport(this->commandBuffer, 0, 1, &this->m_viewPort);
	vkCmdSetScissor(this->commandBuffer, 0, 1, &this->m_scissor);*/

	
	this->mObjectManager["freddy"].Draw(this->commandBuffer);
	this->mObjectManager["base"].Draw(this->commandBuffer);  
	this->mObjectManager["cube"].Draw(this->commandBuffer);


	DrawGui();

	vkCmdEndRenderPass(this->commandBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(this->commandBuffer));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->commandBuffer;

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, this->inFlightFence));

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
		ResizeViewport();
		return;
	}

	assert(result == VK_SUCCESS);

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
		mTime.Update();

		mController.Update();

		mPhysics.Update(mTime.DeltaTime());

		this->mObjectManager["cube"].Update(mPhysics.InterpFactor());
		this->mObjectManager["base"].Update(mPhysics.InterpFactor());
		
		mCamera.Update(mPhysics.InterpFactor());

		mLights.Update();
		

		Render();
	}

	VK_CHECK_RESULT(vkDeviceWaitIdle(this->m_logicalDevice));



}

void Application::DestroyObjects() 
{
	/*debugCube.DestroyResources();
	debugCube2.DestroyResources();
	debugCube3.DestroyResources();*/
}

void Application::exit()
{

	CleanUpGui();

	DestroyObjects();

	vkDestroySemaphore(this->m_logicalDevice, this->imageAvailableSemaphore, nullptr);

	vkDestroySemaphore(this->m_logicalDevice, this->renderFinishedSemaphore, nullptr);

	vkFreeCommandBuffers(this->m_logicalDevice, this->commandPool, 1, &this->commandBuffer);
	
	vkDestroyCommandPool(this->m_logicalDevice, this->commandPool, nullptr);

	for (size_t i = 0; i < this->pipelineLayouts.size(); ++i) 
	{
		vkDestroyPipelineLayout(this->m_logicalDevice, this->pipelineLayouts[i], nullptr);
	}

	
	vkDestroyPipeline(this->m_logicalDevice, this->pipeline, nullptr);

	vkDestroyPipeline(this->m_logicalDevice, this->linePipeline, nullptr);

	vkDestroyDescriptorPool(this->m_logicalDevice, this->descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(this->m_logicalDevice, this->descriptorSetLayout, nullptr);

	for (unsigned i = 0; i < uniformBuffers.size(); ++i) 
	{
		vkDestroyBuffer(this->m_logicalDevice, uniformBuffers[i].handle, nullptr);
		vkFreeMemory(this->m_logicalDevice, uniformBuffers[i].memory, nullptr);
	}
	
	//Texture manager destroy.

	vkDestroyImage(this->m_logicalDevice, this->depthImage, nullptr);
	vkDestroyImageView(this->m_logicalDevice, this->depthImageView, nullptr);
	vkFreeMemory(this->m_logicalDevice, this->depthImageMemory, nullptr);

	vkDestroyShaderModule(this->m_logicalDevice, this->shaderVertModule, nullptr);

	vkDestroyShaderModule(this->m_logicalDevice, this->shaderFragModule, nullptr);
	
	vkDestroyRenderPass(this->m_logicalDevice, this->m_renderPass, nullptr);

	//this already destroys the images in it.
	//vkDestroySwapchainKHR(this->m_logicalDevice, this->swapChain, nullptr);

	vkDestroyFence(this->m_logicalDevice, this->inFlightFence, nullptr);


	//delete[] swapChainImages;
	//delete[] m_physicalDevices;
	
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->m_instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) 
	{
		func(this->m_instance, this->debugMessenger, nullptr);
	}

	mLights.Deallocate();

	mObjectManager.Deallocate();

}


Application::~Application()
{
	vkDestroySurfaceKHR(this->m_instance, this->mWindow.surface, nullptr);

	vkDestroyInstance(this->m_instance, nullptr);
	
}


void Application::CleanUpGui() 
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}





