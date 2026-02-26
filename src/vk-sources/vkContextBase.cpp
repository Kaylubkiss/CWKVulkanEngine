#include "vkContextBase.h"
#include "vkUtility.h"
#include "vkInit.h"

namespace vk
{	

	//constructor
	ContextBase::ContextBase()
	{
		assert(_Application != NULL);

		m_window.Init(640, 480);

		std::vector<const char*> instanceLayers = {"VK_LAYER_KHRONOS_validation"};
		std::vector<const char*> instanceExtensions = m_window.GetInstanceExtensions();
		m_instance.Create(instanceExtensions, instanceLayers);

		m_window.CreateSurface(m_instance.GetHandle());

		
		device.AddExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		device.AddExtension(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
		device.AddExtension(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		device.Init(m_instance.GetHandle(), m_window.Surface());

		swapChain.Init(&this->device, m_window); //need window for its surface and viewport info.
		swapChain.Create(m_window);

		//conforms to higher frame counts to prevent flickering.
		if (swapChain.createInfo.minImageCount > m_settings.maxFramesInFlight)
		{
			m_settings.maxFramesInFlight = swapChain.createInfo.minImageCount;
		}

		ContextBase::InitializeRenderPass();
		this->swapChain.CreateFrameBuffers(m_window.Viewport(), renderPass);

		CreateSynchronizationPrimitives();

		//each swapchain should have its own command buffer
		device.AllocateCommandBuffers(commandBuffers.data(), static_cast<uint32_t>(commandBuffers.size()));

		if (m_settings.UIDisplay)
		{
			UserInterfaceInitInfo userInterfaceCI = {};
			userInterfaceCI.contextInstance = m_instance.GetHandle();
			userInterfaceCI.contextLogicalDevice = this->device.GetDevice();
			userInterfaceCI.contextPhysicalDevice = this->device.GetGPU();
			userInterfaceCI.contextQueue = this->device.GetQueue(DeviceQueue::GRAPHICS);
			userInterfaceCI.contextWindow = m_window.WindowPtr();
			userInterfaceCI.renderPass = this->renderPass;
			userInterfaceCI.minImages = m_settings.maxFramesInFlight;
			userInterfaceCI.viewPortExtent = m_window.Extents();

			this->UIOverlay = UserInterface(userInterfaceCI);
		}

		m_info = std::make_shared<vk::GraphicsContextInfo>();
		ContextBase::FillOutGraphicsContextInfo();

		this->mCamera = Camera({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f }, { 0,1,0 });

		this->pipelineManager.Init(m_info);
	}

	//destructor
	ContextBase::~ContextBase()
	{
		if (device.GetDevice() != VK_NULL_HANDLE)
		{
			pipelineManager.Destroy();
			swapChain.Destroy();
			UIOverlay.Destroy();

			device.FreeCommandBuffers(commandBuffers.data(), static_cast<uint32_t>(commandBuffers.size()));
			//device command pool is deallocated in ~Device()

			//semaphores
			for (int i = 0; i < gMaxFramesInFlight; ++i)
			{
				vkDestroySemaphore(this->device.GetDevice(), presentCompleteSemaphores[i], nullptr);
				vkDestroySemaphore(this->device.GetDevice(), renderCompleteSemaphores[i], nullptr);

				vkDestroyFence(device.GetDevice(), inFlightFences[i], nullptr);
			}

			m_objectManager->Destroy();

			//must destroy the device before instance
			this->device.Destroy();

			vkDestroySurfaceKHR(m_instance.GetHandle(), m_window.Surface(), nullptr);
		}
	}



	//helper(s)
	void ContextBase::CreateSynchronizationPrimitives() 
	{
		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			inFlightFences[i] = vk::init::CreateFence(device.GetDevice(), true);
			presentCompleteSemaphores[i] = vk::init::CreateSemaphore(device.GetDevice());
			renderCompleteSemaphores[i] = vk::init::CreateSemaphore(device.GetDevice());
			textureUploadSemaphores[i] = vk::init::CreateSemaphore(device.GetDevice());
		}
	}

	void ContextBase::ResizeWindow() 
	{
		VK_CHECK_RESULT(vkDeviceWaitIdle(device.GetDevice()));

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetGPU(), m_window.Surface(), &surfaceCapabilities));

		m_window.UpdateExtents(surfaceCapabilities.currentExtent);

		if (m_window.IsMinimized())
		{
			return; //window is minimized, and 0 sizes will cause errors/crashes --> isPrepared will remain false.
		}

		swapChain.Recreate(renderPass, m_window);

		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			vkDestroySemaphore(device.GetDevice(), presentCompleteSemaphores[i], nullptr);
			presentCompleteSemaphores[i] = VK_NULL_HANDLE;

			vkDestroySemaphore(device.GetDevice(), renderCompleteSemaphores[i], nullptr);
			renderCompleteSemaphores[i] = VK_NULL_HANDLE;

			vkDestroyFence(device.GetDevice(), inFlightFences[i], nullptr);
			inFlightFences[i] = VK_NULL_HANDLE;

			vkDestroySemaphore(device.GetDevice(), textureUploadSemaphores[i], nullptr);
			textureUploadSemaphores[i] = VK_NULL_HANDLE;
		}

		CreateSynchronizationPrimitives();

		ResizeWindowDerived();
	}

	void ContextBase::UpdateSceneObjects(float dt) const
	{
		m_objectManager->Update(dt);
	}

	void ContextBase::ToggleUIActive(bool enable)
	{
		m_settings.UIToggled = enable;
	}

	//initializers
	void ContextBase::InitializeRenderPass() 
	{
		assert(swapChain.handle != VK_NULL_HANDLE);

		std::array<VkAttachmentDescription, 2> attachments = {};

		//color attachment
		attachments[0].format = swapChain.createInfo.imageFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		//depth attachment
		attachments[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		//for layout transitions
		std::array<VkSubpassDependency, 2> dependencies{};

		//depth writing/reading dependencies
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		//some pipelines may do the depth test before fragment shader starts, or after,
		//depending on if FragCoord.z is edited in a shader.
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = dependencies[0].srcStageMask;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;

		//color writing/reading dependencies. This is to ensure that the color attachment read/writes are finished before subpass 0 begins and uses them again for reading/writing.
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = dependencies[1].srcStageMask;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //this can also be 0
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[1].dependencyFlags = 0;

		VkRenderPassCreateInfo renderPassCI = {};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassCI.pAttachments = attachments.data();
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpassDescription;
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassCI.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device.GetDevice(), &renderPassCI, nullptr, &renderPass));
	}

	void ContextBase::FillOutGraphicsContextInfo() 
	{
		//TODO: a little janky way to initialize as more of mInfo is filled with derived classes.
		m_info->devicePtr = &this->device;

		if (m_settings.UIDisplay)
		{
			m_info->contextUIPtr = &UIOverlay;
		}
	}

	//getter(s)

	Camera& ContextBase::GetCamera()
	{
		return this->mCamera;
	}

	vk::Window& ContextBase::GetWindow() 
	{
		return m_window;
	}

	std::shared_ptr<GraphicsContextInfo> ContextBase::GetGraphicsContextInfo() const
	{
		return m_info;
	}

	void ContextBase::WaitForDevice() const
	{
		if (device.GetDevice() != VK_NULL_HANDLE)
		{
			VK_CHECK_RESULT(vkDeviceWaitIdle(device.GetDevice()));
		}
	}

	bool ContextBase::PrepareFrame() 
	{
		if (m_window.IsPrepared() == false)
		{
			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetGPU(), m_window.Surface(), &surfaceCapabilities));

			//the question is: CAN the window be resized? If the window's minimized, then we shouldn't proceed with
			//preparing the frame. Otherwise, we should resize the window as ImGui will crash without
			if (surfaceCapabilities.currentExtent.width == 0 && surfaceCapabilities.currentExtent.height == 0)
			{
				return false;
			}
			else
			{
				ResizeWindow();
			}
		}

		VK_CHECK_RESULT(vkWaitForFences(device.GetDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device.GetDevice(), 1, &inFlightFences[currentFrame]));

		if (m_settings.UIDisplay)
		{
			UIOverlay.Prepare();

			VkExtent2D windowExtent = m_window.Extents();

			ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowExtent.width) / 3,
			static_cast<float>(windowExtent.height) / 2 ));

			ImGui::SetNextWindowPos(ImVec2(0,0));

			int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
			if (!m_settings.UIToggled)
			{
				flags |= ImGuiWindowFlags_NoInputs;
			}

			if (ImGui::Begin("CWKVulkanEngine", nullptr, flags) == true)
			{

				UpdateUI();
			}

			ImGui::End();
		}

		VkResult result = 
			vkAcquireNextImageKHR(device.GetDevice(), swapChain.handle, UINT64_MAX,
				presentCompleteSemaphores[currentFrame], (VkFence)nullptr, &currentImageIndex);
	
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR) 
			{
				ResizeWindow();
			}
		}
		else
		{
			VK_CHECK_RESULT(result);
		}


		return result == VK_SUCCESS;
	}

	void ContextBase::SubmitFrame() 
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		std::vector<VkPipelineStageFlags> pipelineWaitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		std::vector<VkSemaphore> waitSemaphores = { presentCompleteSemaphores[currentFrame] };

		bool textureSubmitted = m_objectManager->SyncIO(currentFrame, textureUploadSemaphores[currentFrame]);
		if (textureSubmitted == true)
		{
			pipelineWaitStages.push_back(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			waitSemaphores.push_back(textureUploadSemaphores[currentFrame]);
		}

		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = pipelineWaitStages.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentImageIndex];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &this->commandBuffers[currentFrame];

		VK_CHECK_RESULT(vkQueueSubmit(device.GetQueue(DeviceQueue::GRAPHICS).handle, 1, &submitInfo,
			inFlightFences[currentFrame]));

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentImageIndex];
		presentInfo.pImageIndices = &currentImageIndex;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &this->swapChain.handle;

		VkResult result = vkQueuePresentKHR(device.GetQueue(DeviceQueue::PRESENT).handle, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR) 
			{
				ResizeWindow();
			}
		}
		else
		{
			VK_CHECK_RESULT(result);
		}

		//we need to ensure that the queue submission completes.
		if (textureSubmitted == false)
		{
			currentFrame = (currentFrame + 1) % m_settings.maxFramesInFlight;
		}
	}

}