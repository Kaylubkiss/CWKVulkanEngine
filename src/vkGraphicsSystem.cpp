#include "vkGraphicsSystem.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <stdexcept>
#include "ApplicationGlobal.h"
#include "ObjectManager.h"


namespace vk
{	

	//extern variables!!!

	void GraphicsSystem::Destroy() 
	{
		if (this->isInitialized) 
		{
			mPipeline.Destroy(this->logicalGpu);
			swapChain.Destroy(this->logicalGpu);
			renderResources.Destroy(this->logicalGpu);

			//buffers
			uTransformBuffer.Destroy(this->logicalGpu);
			uLightBuffer.Destroy(this->logicalGpu);

			//semaphores
			vkDestroySemaphore(this->logicalGpu, semaphores.presentComplete, nullptr);
			vkDestroySemaphore(this->logicalGpu, semaphores.renderComplete, nullptr);

			vkDestroyDevice(this->logicalGpu, nullptr);
		}

	}

	GraphicsSystem::GraphicsSystem(const VkInstance vkInstance, const vk::Window& appWindow)
	{

		assert(_Application != NULL);

		if (vkInstance == VK_NULL_HANDLE) {

			throw std::runtime_error("Can't create graphics without instance!");
		}

		GraphicsSystem::EnumeratePhysicalDevices(vkInstance);
		GraphicsSystem::FindQueueFamilies(this->gpus[g_index], appWindow.surface);

		this->logicalGpu = GraphicsSystem::CreateLogicalDevice(this->gpus[g_index], graphicsQueue.family, presentQueue.family);

		vkGetDeviceQueue(this->logicalGpu, graphicsQueue.family, 0, &graphicsQueue.handle);
		vkGetDeviceQueue(this->logicalGpu, presentQueue.family, 0, &presentQueue.handle);

		semaphores.presentComplete = vk::init::CreateSemaphore(this->logicalGpu);
		semaphores.renderComplete = vk::init::CreateSemaphore(this->logicalGpu);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &pipelineWaitStages;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphores.renderComplete;

		this->renderResources.Allocate(this->gpus[g_index], this->logicalGpu, appWindow);

		GraphicsSystem::InitializedUniforms(appWindow);

		GraphicsSystem::InitializePipeline(appWindow, "shadowMapForward.vert", "shadowMapForward.frag");

		this->swapChain = SwapChain(this->logicalGpu, this->gpus[g_index], graphicsQueue.family, presentQueue.family, appWindow.surface);

		/* NOTE: a bit jank, as swapchain relies on Finalize method of mPipeline to finish */
		this->swapChain.AllocateFrameBuffers(this->logicalGpu, appWindow.viewport, this->mPipeline.RenderDepthInfo(), this->mPipeline.RenderPass());

		this->isInitialized = true;
	}
	
	const VkPhysicalDevice GraphicsSystem::PhysicalDevice() const
	{
		return this->gpus[g_index];
	}

	const VkDevice GraphicsSystem::LogicalDevice() const 
	{
		return this->logicalGpu;
	}

	vk::Queue GraphicsSystem::GraphicsQueue() 
	{
		return this->graphicsQueue;
	}

	VkRenderPass GraphicsSystem::RenderPass() 
	{
		return this->mPipeline.RenderPass();
	}

	const VkPipeline GraphicsSystem::Pipeline() const 
	{
		return this->mPipeline.Handle();
	}

	const VkDescriptorSetLayout GraphicsSystem::DescriptorSetLayout() const 
	{
		//TODO: support different descriptorsetlayouts.
		return this->mPipeline.DescriptorSetLayout();
	}
	
	const vk::Buffer& GraphicsSystem::UTransformBuffer() 
	{
		return this->uTransformBuffer;
	}

	const vk::Buffer& GraphicsSystem::ULightBuffer() 
	{
		return this->uLightBuffer;
	}

	void GraphicsSystem::FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface)
	{

		uint32_t queueFamilyPropertyCount;
		std::vector<VkQueueFamilyProperties> queueFamilies;

		//no use for memory properties right now.
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

		//similar maneuver to vkEnumeratePhysicalDevices
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

		if (queueFamilyPropertyCount == 0)
		{
			throw std::runtime_error("couldn't find any queue families...");
		}

		queueFamilies.resize(queueFamilyPropertyCount);

		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilies.data());

		bool setGraphicsQueue = false;
		bool setPresentQueue = false;

		for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
		{
			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
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
	}

	void GraphicsSystem::EnumeratePhysicalDevices(const VkInstance& vkInstance)
	{
		//list the physical devices
		uint32_t max_devices = 0;

		//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices 
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &max_devices, nullptr))

		if (!max_devices)
		{
			throw std::runtime_error("could not find any GPUs to use!\n");
		}

		this->gpus.resize(max_devices);

		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &max_devices, this->gpus.data()))

		for (size_t i = 0; i < max_devices; ++i)
		{
			
			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;

			vkGetPhysicalDeviceProperties(this->gpus[i], &properties);
			vkGetPhysicalDeviceFeatures(this->gpus[i], &features);


			if ((properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || 
			properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) && 
			features.geometryShader && features.samplerAnisotropy)
			{
				std::cout << "picked device " << i << '\n';

				g_index = i;
				break;
			}
		}

		if (g_index < 0)
		{
			throw std::runtime_error("could not find suitable physical device!");
		}


	}

	VkDevice GraphicsSystem::CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily)
	{
		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos; //presentation and graphics.

		uint32_t uniqueQueueFamilies[2] = { graphicsFamily, presentFamily };
		
		float queuePriority[1] = {1.f};

		if (graphicsFamily != presentFamily) 
		{
			for (unsigned i = 0; i < 2; ++i)
			{
				VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.
				deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				deviceQueueInfo.flags = 0;
				deviceQueueInfo.pNext = nullptr;
				deviceQueueInfo.queueFamilyIndex = uniqueQueueFamilies[i];
				deviceQueueInfo.queueCount = 1;
				//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
				deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

				deviceQueueCreateInfos.push_back(deviceQueueInfo);
			}
		}
		else {
			VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.
			deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueInfo.flags = 0;
			deviceQueueInfo.pNext = nullptr;
			deviceQueueInfo.queueFamilyIndex = graphicsFamily;
			deviceQueueInfo.queueCount = 1;
			//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
			deviceQueueInfo.pQueuePriorities = queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

			deviceQueueCreateInfos.push_back(deviceQueueInfo);

		}

		//won't do many other optional features for now.
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.geometryShader = VK_TRUE;
	/*	deviceFeatures.tessellationShader = VK_TRUE;*/
		deviceFeatures.samplerAnisotropy = VK_TRUE;


		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pNext = nullptr;

		//maybe don't assume extensions are there!!!!	
		deviceCreateInfo.enabledExtensionCount = sizeof(deviceExtensions)/sizeof(deviceExtensions[0]);
		deviceCreateInfo.ppEnabledExtensionNames = vk::deviceExtensions;
		
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures; //call vkGetPhysicalDeviceFeatures to set additional features.

		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = (uint32_t)(deviceQueueCreateInfos.size());

		VkDevice nLogicalDevice;
		VK_CHECK_RESULT(vkCreateDevice(p_device, &deviceCreateInfo, nullptr, &nLogicalDevice));

		return nLogicalDevice;
	}


	void GraphicsSystem::ResizeWindow() 
	{
		assert(_Application != NULL);

		VK_CHECK_RESULT(vkDeviceWaitIdle(this->logicalGpu));
		
		vk::Window& appWindow = _Application->GetWindow();

		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->gpus[g_index], appWindow.surface, &deviceCapabilities));

		appWindow.UpdateExtents(deviceCapabilities.currentExtent);
		this->renderResources.currentExtent = deviceCapabilities.currentExtent;

		//updating the uniform projection matrix after updating the viewport size
		uTransform.proj = glm::perspective(glm::radians(45.f), (float)appWindow.viewport.width / appWindow.viewport.height, 0.1f, 1000.f); //proj
		
		uTransform.proj[1][1] *= -1.f;		
		
		memcpy(uTransformBuffer.mappedMemory, (void*)&uTransform, uTransformBuffer.size);

		this->swapChain.Recreate(this->gpus[g_index], this->logicalGpu, this->graphicsQueue.family, this->presentQueue.family, mPipeline.RenderDepthInfo(), this->mPipeline.RenderPass(), appWindow);


	}

	void GraphicsSystem::InitializedUniforms(const vk::Window& appWindow)
	{
		Camera& appCamera = _Application->GetCamera();

		//uniform transform for objects of default pipeline.
		this->uTransform = {
			appCamera.LookAt(), //view
			glm::perspective(glm::radians(45.f), (float)appWindow.viewport.width / appWindow.viewport.height, 0.1f, 1000.f) //proj
		};

		this->uTransform.proj[1][1] *= -1.f;

		//uniform(s)
		this->uTransformBuffer = vk::Buffer(gpus[g_index], logicalGpu, sizeof(uTransformObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)&this->uTransform);

		this->uLight.pos = {};
		this->uLight.albedo = { 1.0, 1.0, 1.0 };
		this->uLight.ambient = this->uLight.albedo * 0.1f;
		this->uLight.specular = { 0.5f, 0.5f, 0.5f };
		this->uLight.shininess = 16.f;


		this->uLightBuffer = vk::Buffer(this->gpus[g_index], this->logicalGpu, sizeof(uLightObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)(&this->uLight));

	}

	void GraphicsSystem::InitializePipeline(const vk::Window& appWindow, std::string vsFile, std::string fsFile)
	{
		ShaderModuleInfo vertColorInfo(this->logicalGpu, vsFile, VK_SHADER_STAGE_VERTEX_BIT);

		ShaderModuleInfo fragColorInfo(this->logicalGpu, fsFile, VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);


		VkDescriptorSetLayoutBinding uTransformBinding{};
		uTransformBinding.binding = 0;
		uTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uTransformBinding.descriptorCount = 1; //one uniform struct.
		uTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //we are going to use the transforms in the vertex shader.

		VkDescriptorSetLayoutBinding samplerBinding = {};
		samplerBinding.binding = 1;
		samplerBinding.descriptorCount = 1;
		samplerBinding.pImmutableSamplers = nullptr;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //we are going to use the sampler in the fragment shader.

		VkDescriptorSetLayoutBinding uLightBinding = {};
		uLightBinding.binding = 2;
		uLightBinding.descriptorCount = 1;
		uLightBinding.pImmutableSamplers = nullptr;
		uLightBinding.descriptorCount = 1;
		uLightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uLightBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		std::vector<VkDescriptorSetLayoutBinding> dscSetLayoutBindings = { uTransformBinding, samplerBinding, uLightBinding };

		VkDescriptorSetLayout colorSetLayout = vk::init::DescriptorSetLayout(this->logicalGpu, dscSetLayoutBindings);


		//this is for an object's model transformation.
		std::vector<VkPushConstantRange> pushConstants(1);
		pushConstants[0].offset = 0;
		pushConstants[0].size = sizeof(glm::mat4);
		pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


		VkPipelineLayout colorPipelineLayout = vk::init::CreatePipelineLayout(this->logicalGpu, colorSetLayout, pushConstants);

		this->mPipeline.AddModule(vertColorInfo).
			AddModule(fragColorInfo).
			AddDescriptorSetLayout(colorSetLayout).
			AddPipelineLayout(colorPipelineLayout).
			Finalize(this->logicalGpu, this->gpus[g_index], appWindow, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
			);

	}


	VkCommandPool GraphicsSystem::CommandPool() 
	{
		return this->renderResources.commandPool;
	}

	void GraphicsSystem::BindPipelineLayoutToObject(Object& obj) 
	{
		obj.UpdatePipelineLayout(this->mPipeline.Layout());
	}

	void GraphicsSystem::WaitForDevice() 
	{
		if (this->isInitialized) 
		{
			VK_CHECK_RESULT(vkDeviceWaitIdle(this->logicalGpu));
		}
	}

	void GraphicsSystem::UpdateUniformViewMatrix(const glm::mat4& viewMat)
	{
		uTransform.view = viewMat;

		memcpy(uTransformBuffer.mappedMemory, (void*)&uTransform, static_cast<VkDeviceSize>(sizeof(uTransformObject)));
	}

	void GraphicsSystem::BuildCommandBuffers(ObjectManager& objManager)
	{
		for (size_t i = 0; i < this->renderResources.commandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			//everything else is default...

			//resetting command buffer should be implicit with reset flag set on this.
			VK_CHECK_RESULT(vkBeginCommandBuffer(this->renderResources.commandBuffers[i], &cmdBufferBeginInfo))

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = this->mPipeline.RenderPass();
			renderPassInfo.framebuffer = this->swapChain.frameBuffers[i];
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = this->renderResources.currentExtent;

			VkClearValue clearColors[2] = {};
			clearColors[0].color = { {0.5f, 0.5f, 0.5f, 1.f} };
			clearColors[1].depthStencil = { 1.f, 0 };

			renderPassInfo.clearValueCount = 2;
			renderPassInfo.pClearValues = clearColors;

			vkCmdBeginRenderPass(this->renderResources.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(this->renderResources.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->mPipeline.Handle());

			const vk::Window& appWindow = _Application->GetWindow();

			vkCmdSetViewport(this->renderResources.commandBuffers[i], 0, 1, &appWindow.viewport);
			vkCmdSetScissor(this->renderResources.commandBuffers[i], 0, 1, &appWindow.scissor);

			objManager.DrawObjects(this->renderResources.commandBuffers[i]);

			vkCmdEndRenderPass(this->renderResources.commandBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(this->renderResources.commandBuffers[i]));
		}

	}

	void GraphicsSystem::Render(const vk::Window& appWindow, VkCommandBuffer* secondCmdBuffers, size_t secondCmdCount)
	{
		//wait for queue submission..
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(this->logicalGpu, this->swapChain.handle, UINT64_MAX, semaphores.presentComplete, (VkFence)nullptr, &imageIndex);


		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			GraphicsSystem::ResizeWindow();
			return;
		}
		else 
		{
			assert(result == VK_SUCCESS);
		}

		submitInfo.commandBufferCount = 1;


		submitInfo.pCommandBuffers = &this->renderResources.commandBuffers[imageIndex];

		VK_CHECK_RESULT(vkQueueSubmit(this->graphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE))

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &this->swapChain.handle;
		presentInfo.pImageIndices = &imageIndex;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &semaphores.renderComplete;

		result = vkQueuePresentKHR(this->presentQueue.handle, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			GraphicsSystem::ResizeWindow();
			return;
		}
		else 
		{
			assert(result == VK_SUCCESS);
		}

		VK_CHECK_RESULT(vkQueueWaitIdle(this->presentQueue.handle));
	}

}