#include "vkRendererBase.h"
#include "vkUtil.h"
#include "vkInit.h"

namespace vk
{	

	//constructor
	RendererBase::RendererBase()
	{
		m_window = vk::Window(640, 480);

		std::vector<const char*> instanceLayers = {"VK_LAYER_KHRONOS_validation"};
		std::vector<const char*> instanceExtensions = m_window.GetInstanceExtensions();
		m_instance = vk::Instance(instanceExtensions, instanceLayers);

		m_window.CreateSurface(m_instance.GetHandle());

		std::vector<const char*> deviceExtensions =
		{
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
				VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
			}
		};
		device = vk::Device(m_instance.GetGPU(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU),
			m_window.Surface(), deviceExtensions);

		swapChain.Init(&this->device, m_window); //need window for its surface and viewport info.
		swapChain.Create(m_window);

		//conforms to higher frame counts to prevent flickering.
		m_settings.maxFramesInFlight = std::max(swapChain.createInfo.minImageCount, m_settings.maxFramesInFlight);

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
			userInterfaceCI.renderPass = swapChain.renderPass;
			userInterfaceCI.minImages = m_settings.maxFramesInFlight;
			userInterfaceCI.viewPortExtent = m_window.Extents();

			m_settings.UIToggled = true;

			this->UIOverlay = UserInterface(userInterfaceCI);
		}

		RendererBase::FillOutGraphicsContextInfo();

		this->mCamera = Camera({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f }, { 0,1,0 });

		this->pipelineManager.Init(m_info);
	}

	//destructor
	RendererBase::~RendererBase()
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
				vkDestroySemaphore(this->device.GetDevice(), textureUploadSemaphores[i], nullptr);

				vkDestroyFence(device.GetDevice(), inFlightFences[i], nullptr);
			}

			vkDestroySurfaceKHR(m_instance.GetHandle(), m_window.Surface(), nullptr);
		}
	}

	//helper(s)
	void RendererBase::CreateSynchronizationPrimitives()
	{
		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			inFlightFences[i] = vk::init::CreateFence(device.GetDevice(), true);
			presentCompleteSemaphores[i] = vk::init::CreateSemaphore(device.GetDevice());
			renderCompleteSemaphores[i] = vk::init::CreateSemaphore(device.GetDevice());
			textureUploadSemaphores[i] = vk::init::CreateSemaphore(device.GetDevice());
		}
	}

	void RendererBase::ResizeWindow()
	{
		VK_CHECK_RESULT(vkDeviceWaitIdle(device.GetDevice()));

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetGPU(), m_window.Surface(), &surfaceCapabilities));

		m_window.UpdateExtents(surfaceCapabilities.currentExtent);

		if (m_window.IsMinimized())
		{
			return; //window is minimized, and 0 sizes will cause errors/crashes --> isPrepared will remain false.
		}

		swapChain.Recreate( m_window );

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
	}

	void RendererBase::ToggleUIActive(bool enable)
	{
		m_settings.UIToggled = enable;
	}

	//initializers
	void RendererBase::FillOutGraphicsContextInfo()
	{
		//TODO: a little janky way to initialize as more of mInfo is filled with derived classes.
		m_info.devicePtr = &this->device;
	}

	//getter(s)

	Camera& RendererBase::GetCamera()
	{
		return this->mCamera;
	}

	vk::Window& RendererBase::GetWindow()
	{
		return m_window;
	}

	GraphicsContextInfo& RendererBase::GetInfo()
	{
		return m_info;
	}

	void RendererBase::WaitForDevice() const
	{
		if (device.GetDevice() != VK_NULL_HANDLE)
		{
			VK_CHECK_RESULT(vkDeviceWaitIdle(device.GetDevice()));
		}
	}

	bool RendererBase::PrepareFrame()
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

		if (m_settings.hotReloadRequested == true)
		{
			pipelineManager.HotReloadShaders();

			m_settings.hotReloadRequested = false;
		}


		if (m_settings.UIDisplay)
		{
			UIOverlay.Prepare();

			VkExtent2D windowExtent = m_window.Extents();

			ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowExtent.width) / 3,
			static_cast<float>(windowExtent.height) / 2 ));

			ImGui::SetNextWindowPos(ImVec2(0,0));

			int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

			if (m_settings.UIToggled == false)
			{
				flags |= ImGuiWindowFlags_NoInputs;
			}

			if (ImGui::Begin("CWKVulkanEngine", nullptr, flags) == true)
			{
				UIOverlay.TextData("FPS: %d", static_cast<int>(_Timer.GetFPS()));
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

		pipelineManager.DetectHotReloadableShaders();

		return result == VK_SUCCESS;

	}

	void RendererBase::SubmitFrame()
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		std::vector<VkPipelineStageFlags> pipelineWaitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		std::vector<VkSemaphore> waitSemaphores = { presentCompleteSemaphores[currentFrame] };

		if (m_textureManagerPtr)
		{
			bool textureSubmitted = m_textureManagerPtr->UploadTextureDataToGPU(currentFrame, textureUploadSemaphores[currentFrame]);
			if (textureSubmitted == true)
			{
				pipelineWaitStages.push_back(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
				waitSemaphores.push_back(textureUploadSemaphores[currentFrame]);
			}
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

		currentFrame = (currentFrame + 1) % m_settings.maxFramesInFlight;
	}

}