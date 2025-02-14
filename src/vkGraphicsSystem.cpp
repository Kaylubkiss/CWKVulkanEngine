#include "vkGraphicsSystem.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <stdexcept>
#include "Camera.h"


namespace vk
{
	static const std::string shaderPath{ "Shaders/" };

	void RenderResources::Destroy(const VkDevice l_device) 
	{
		//pipeline info...
		vkDestroyPipelineLayout(l_device, this->defaultPipelineLayout, nullptr);
		vkDestroyPipeline(l_device, this->defaultPipeline, nullptr);


		this->depthInfo.Destroy(l_device);
		
		vkDestroyRenderPass(l_device, this->renderPass, nullptr);
		
		vkDestroyFence(l_device, this->inFlightFence, nullptr);

		//command pools and their handles.
		vkFreeCommandBuffers(l_device, this->commandPool, 1, &this->commandBuffer);
		vkDestroyCommandPool(l_device, this->commandPool, nullptr);
		
		//semaphores
		vkDestroySemaphore(l_device, this->imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(l_device, this->renderFinishedSemaphore, nullptr);

		//shader modules
		vkDestroyShaderModule(l_device, this->vertexShaderModule, nullptr);
		vkDestroyShaderModule(l_device, this->fragmentShaderModule, nullptr);

		//buffers
		vkDestroyBuffer(l_device, uniformBuffer.handle, nullptr);
		vkFreeMemory(l_device, uniformBuffer.memory, nullptr);
	}

	void RenderResources::Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow)
	{
		assert(_Application != NULL);

		Camera& appCamera = _Application->GetCamera();

		this->depthInfo = vk::init::CreateDepthResources(p_device, l_device, appWindow.viewport);

		this->renderPass = vk::init::RenderPass(l_device, depthInfo.depthFormat);

		this->commandPool = vk::init::CommandPool(l_device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		this->commandBuffer = vk::init::CommandBuffer(l_device, this->commandPool);

		//uniform transform for objects of default pipeline.
		this->uTransform = {
		glm::mat4(1.f), //model
		appCamera.LookAt(), //view
		glm::perspective(glm::radians(45.f), (float)appWindow.viewport.width / appWindow.viewport.height, 0.1f, 1000.f) //proj
		};

		this->uTransform.proj[1][1] *= -1.f;

		//uniform(s)
		this->uniformBuffer = vk::Buffer(p_device, l_device, sizeof(uTransformObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)&uTransform);

		//semaphores
		this->imageAvailableSemaphore = vk::init::CreateSemaphore(l_device);
		this->renderFinishedSemaphore = vk::init::CreateSemaphore(l_device);
		
		//fence(s)
		this->inFlightFence = vk::init::CreateFence(l_device);

		//create pipeline
		this->defaultDescriptorSetLayout = vk::init::DescriptorSetLayout(l_device);
		
		this->defaultPipelineLayout = vk::init::CreatePipelineLayout(l_device, this->defaultDescriptorSetLayout);

		//create shader modules...
		std::string vertexShaderPath = shaderPath + "blinnvert.spv";
		this->vertexShaderModule = vk::init::ShaderModule(l_device, vertexShaderPath.data());

		std::string fragmentShaderPath = shaderPath + "blinnfrag.spv";
		this->fragmentShaderModule = vk::init::ShaderModule(l_device, fragmentShaderPath.data());


		VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = vk::init::PipelineShaderStageCreateInfo(this->vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT);
		VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = vk::init::PipelineShaderStageCreateInfo(this->fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT);

		VkPipelineShaderStageCreateInfo shaderStages[2] = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

		this->defaultPipeline = vk::init::CreatePipeline(l_device, this->defaultPipelineLayout, this->renderPass, shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		//window sizing...
		VkSurfaceCapabilitiesKHR deviceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, appWindow.surface, &deviceCapabilities);

		this->currentExtent = deviceCapabilities.currentExtent;
	}

	void GraphicsSystem::Destroy() 
	{

		VK_CHECK_RESULT(vkDeviceWaitIdle(this->logicalGpu));

		renderResources.Destroy(this->logicalGpu);
		swapChain.Destroy(this->logicalGpu);

		vkDestroyDevice(this->logicalGpu, nullptr);
		delete[] gpus;

	}

	GraphicsSystem::GraphicsSystem(const VkInstance vkInstance, const vk::Window& appWindow)
	{
		if (vkInstance == VK_NULL_HANDLE) {

			throw std::runtime_error("Can't create graphics without instance!");
		}

		GraphicsSystem::EnumeratePhysicalDevices(vkInstance);
		GraphicsSystem::FindQueueFamilies(this->gpus[g_index], appWindow.surface);

		this->logicalGpu = GraphicsSystem::CreateLogicalDevice(this->gpus[g_index], graphicsQueue.family, presentQueue.family);

		vkGetDeviceQueue(this->logicalGpu, graphicsQueue.family, 0, &graphicsQueue.handle);
		vkGetDeviceQueue(this->logicalGpu, presentQueue.family, 0, &presentQueue.handle);

		this->renderResources.Allocate(this->gpus[g_index], this->logicalGpu, appWindow);

		this->swapChain = SwapChain(this->logicalGpu, this->gpus[g_index], graphicsQueue.family, presentQueue.family, appWindow.surface);
		this->swapChain.AllocateFrameBuffers(this->logicalGpu, appWindow.viewport, this->renderResources.depthInfo, this->renderResources.renderPass);

		
		//special thing here, for dynamic viewport, which is officially set during pipeline creation.
		/*vkCmdSetViewport(this->renderResources.commandBuffer, 0, 1, &appWindow.viewport);
		vkCmdSetScissor(this->renderResources.commandBuffer, 0, 1, &appWindow.scissor);*/

	}
	
	const VkPhysicalDevice GraphicsSystem::PhysicalDevice() const
	{
		return this->gpus[g_index];
	}

	void GraphicsSystem::FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface)
	{

		uint32_t queueFamilyPropertyCount;
		VkQueueFamilyProperties* queueFamilies = nullptr;

		//no use for memory properties right now.
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

		//similar maneuver to vkEnumeratePhysicalDevices
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

		if (queueFamilyPropertyCount == 0)
		{
			throw std::runtime_error("couldn't find any queue families...");
		}

		queueFamilies = new VkQueueFamilyProperties[queueFamilyPropertyCount];

		if (queueFamilies == nullptr)
		{
			throw std::runtime_error("couldn't allocate queueFamilies array\n");
		}

		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilies);

		bool setGraphicsQueue = false;
		bool setPresentQueue = false;

		for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
		{
			if (((*(queueFamilies + i)).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				graphicsQueue.family = i;
				setGraphicsQueue = true;
			}


			VkBool32 presentSupport = false;
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport));

			if (presentSupport)
			{
				presentQueue.family = i;
				setPresentQueue = true;
			}

			if (setGraphicsQueue && setPresentQueue)
			{
				break;
			}

		}

		delete[] queueFamilies;
	}

	void GraphicsSystem::EnumeratePhysicalDevices(const VkInstance& vkInstance)
	{
		//list the physical devices
		uint32_t max_devices = 0;

		//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices 
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &max_devices, nullptr))

		if (max_devices <= 0)
		{
			throw std::runtime_error("could not find any GPUs to use!\n");
			return;
		}


		this->gpus = new VkPhysicalDevice[max_devices];

		if (this->gpus == NULL)
		{
			throw std::runtime_error("could not allocate array of physical devices\n");
		}

		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &max_devices, this->gpus))

		for (size_t i = 0; i < max_devices; ++i)
		{
			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;

			vkGetPhysicalDeviceProperties(*(this->gpus + i), &properties);
			vkGetPhysicalDeviceFeatures(*(this->gpus + i), &features);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
			features.geometryShader && features.samplerAnisotropy)
			{
				g_index = i;
			}
		}

		if (g_index < 0)
		{
			throw std::runtime_error("could not find suitable physical device!");
		}


	}

	VkDevice GraphicsSystem::CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfos[2]; //presentation and graphics.

		uint32_t uniqueQueueFamilies[2] = { graphicsFamily, presentFamily };

		for (unsigned i = 0; i < 2; ++i)
		{
			VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.
			deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueInfo.flags = 0;
			deviceQueueInfo.pNext = nullptr;
			deviceQueueInfo.queueFamilyIndex = uniqueQueueFamilies[i];
			deviceQueueInfo.queueCount = 1;
			float queuePriority = 1.f;
			//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
			deviceQueueInfo.pQueuePriorities = &queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

			deviceQueueCreateInfos[i] = deviceQueueInfo;
		}

		//won't do many other optional features for now.
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.geometryShader = VK_TRUE;
		deviceFeatures.tessellationShader = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pNext = nullptr;

		//layers are no right now.

		deviceCreateInfo.enabledLayerCount = 1;
		deviceCreateInfo.ppEnabledLayerNames = enabledLayerNames;

		deviceCreateInfo.enabledExtensionCount = 1;
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

		deviceCreateInfo.pEnabledFeatures = &deviceFeatures; //call vkGetPhysicalDeviceFeatures to set additional features.

		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;
		deviceCreateInfo.queueCreateInfoCount = 1;

		VkDevice nLogicalDevice;
		VK_CHECK_RESULT(vkCreateDevice(p_device, &deviceCreateInfo, nullptr, &nLogicalDevice));

		return nLogicalDevice;
	}

	const VkDevice GraphicsSystem::LogicalDevice() const 
	{
		return this->logicalGpu;
	}

	void GraphicsSystem::UpdateUniformViewMatirx(const glm::mat4& viewMat) 
	{
		this->renderResources.uTransform.view = viewMat;

		memcpy(this->renderResources.uniformBuffer.mappedMemory, (void*)&this->renderResources.uTransform, (size_t)(sizeof(uTransformObject)));

	}

	void GraphicsSystem::ResizeWindow() 
	{
		assert(_Application != NULL);
		vk::Window& appWindow = _Application->GetWindow();

 		VK_CHECK_RESULT(vkDeviceWaitIdle(this->logicalGpu));

		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->gpus[g_index], appWindow.surface, &deviceCapabilities));

		appWindow.UpdateExtents(deviceCapabilities.currentExtent);

		this->renderResources.currentExtent = deviceCapabilities.currentExtent;

		//updating the uniform projection matrix after updating the viewport size
		this->renderResources.uTransform.proj = glm::perspective(glm::radians(45.f), (float)appWindow.viewport.width / appWindow.viewport.height, 0.1f, 1000.f); //proj
			this->renderResources.uTransform.proj[1][1] *= -1.f;		
		memcpy(this->renderResources.uniformBuffer.mappedMemory, (void*)&this->renderResources.uTransform, (size_t)(sizeof(uTransformObject)));

		this->swapChain.Recreate(this->gpus[g_index], this->logicalGpu, this->graphicsQueue.family, this->presentQueue.family, this->renderResources.depthInfo, this->renderResources.renderPass, appWindow);
	}


	void GraphicsSystem::Render(const vk::Window& appWindow) 
	{
		VK_CHECK_RESULT(vkWaitForFences(this->logicalGpu, 1, &this->renderResources.inFlightFence, VK_TRUE, UINT64_MAX))

		VK_CHECK_RESULT(vkResetFences(this->logicalGpu, 1, &this->renderResources.inFlightFence))

			uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(this->logicalGpu, this->swapChain.handle, UINT64_MAX, this->renderResources.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);


		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			GraphicsSystem::ResizeWindow();
			return;
		}

		assert(result == VK_SUCCESS);

		VK_CHECK_RESULT(vkResetCommandBuffer(this->renderResources.commandBuffer, 0))

			////always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
			VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		//everything else is default...

		//resetting command buffer should be implicit with reset flag.
		VK_CHECK_RESULT(vkBeginCommandBuffer(this->renderResources.commandBuffer, &cmdBufferBeginInfo))

		
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = this->renderResources.renderPass;
		renderPassInfo.framebuffer = this->swapChain.frameBuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = this->renderResources.currentExtent;

		VkClearValue clearColors[2] = {};
		clearColors[0].color = { {0.f, 0.f, 0.f, 1.f} };
		clearColors[1].depthStencil = { 1.f, 0 };

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColors;

		//put this in a draw frame
		vkCmdBeginRenderPass(this->renderResources.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//bind the graphics pipeline
		vkCmdBindPipeline(this->renderResources.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->renderResources.defaultPipeline);

		vkCmdSetViewport(this->renderResources.commandBuffer, 0, 1, &appWindow.viewport);
		vkCmdSetScissor(this->renderResources.commandBuffer, 0, 1, &appWindow.scissor);


		//this->mObjectManager["freddy"].Draw(this->commandBuffer);
		//this->mObjectManager["base"].Draw(this->commandBuffer);
		//this->mObjectManager["cube"].Draw(this->commandBuffer);


		/*DrawGui();*/

		vkCmdEndRenderPass(this->renderResources.commandBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(this->renderResources.commandBuffer));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { this->renderResources.imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &this->renderResources.commandBuffer;

		VkSemaphore signalSemaphores[] = { this->renderResources.renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VK_CHECK_RESULT(vkQueueSubmit(this->graphicsQueue.handle, 1, &submitInfo, this->renderResources.inFlightFence))

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { this->swapChain.handle };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(this->presentQueue.handle, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			GraphicsSystem::ResizeWindow();
			return;
		}

		assert(result == VK_SUCCESS);

	}

}