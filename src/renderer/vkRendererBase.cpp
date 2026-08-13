#include "vkRendererBase.h"
#include "vkUtil.h"
#include "vkInit.h"

namespace vk
{	

	//constructor
	RendererBase::RendererBase( vk::Device& device, vk::Window& window, vk::TextureManager& textureManager ) :
		c_device(device), c_window(window), c_textureManager(textureManager)
	{
		swapChain = vk::SwapChain( &c_device, c_window ); //need window for its surface and viewport info.

		CreateSynchronizationPrimitives();

		//each swapchain should have its own command buffer
		c_device.AllocateCommandBuffers(commandBuffers.data(), static_cast<uint32_t>(commandBuffers.size()));

		if ( m_settings.UIDisplay )
		{
			UserInterfaceInitInfo userInterfaceCI = {};
			userInterfaceCI.contextInstance = c_window.GetContextInstance();
			userInterfaceCI.contextLogicalDevice = c_device.GetDevice();
			userInterfaceCI.contextPhysicalDevice = c_device.GetGPU();
			userInterfaceCI.contextQueue = c_device.GetQueue(DeviceQueue::GRAPHICS);
			userInterfaceCI.contextWindow = c_window.WindowPtr();
			userInterfaceCI.renderPass = swapChain.GetRenderPass();
			userInterfaceCI.minImages = m_settings.maxFramesInFlight;
			userInterfaceCI.viewPortExtent = c_window.Extents();

			UserInterface::Init( userInterfaceCI );
		}

		this->pipelineManager = vk::PipelineManager(device);
	}

	//destructor
	RendererBase::~RendererBase()
	{
		UserInterface::Destroy();

		c_device.FreeCommandBuffers(commandBuffers.data(), static_cast<uint32_t>(commandBuffers.size()));

		//semaphores
		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			vkDestroySemaphore(c_device.GetDevice(), presentCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(c_device.GetDevice(), renderCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(c_device.GetDevice(), textureUploadSemaphores[i].graphicsSubmitSemaphore, nullptr);
			vkDestroySemaphore(c_device.GetDevice(), textureUploadSemaphores[i].transferSubmitSemaphore, nullptr);

			vkDestroyFence(c_device.GetDevice(), inFlightFences[i], nullptr);
		}
	}

	//helper(s)
	void RendererBase::CreateSynchronizationPrimitives()
	{
		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			inFlightFences[i] = vk::init::CreateFence(c_device.GetDevice(), true);
			presentCompleteSemaphores[i] = vk::init::CreateSemaphore(c_device.GetDevice());
			renderCompleteSemaphores[i] = vk::init::CreateSemaphore(c_device.GetDevice());
			textureUploadSemaphores[i].graphicsSubmitSemaphore = vk::init::CreateSemaphore(c_device.GetDevice());
			textureUploadSemaphores[i].transferSubmitSemaphore = vk::init::CreateSemaphore(c_device.GetDevice());
		}
	}

	void RendererBase::ResizeWindow()
	{
		VK_CHECK_RESULT( vkDeviceWaitIdle(c_device.GetDevice()) );

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR( c_device.GetGPU(), c_window.Surface(),
			&surfaceCapabilities ));

		c_window.UpdateExtents( surfaceCapabilities.currentExtent );

		if ( c_window.IsMinimized() )
		{
			return; //window is minimized, and 0 sizes will cause errors/crashes --> isPrepared will remain false.
		}

		swapChain.Recreate( c_window );

		for (int i = 0; i < gMaxFramesInFlight; ++i)
		{
			vkDestroySemaphore(c_device.GetDevice(), presentCompleteSemaphores[i], nullptr);
			presentCompleteSemaphores[i] = VK_NULL_HANDLE;

			vkDestroySemaphore(c_device.GetDevice(), renderCompleteSemaphores[i], nullptr);
			renderCompleteSemaphores[i] = VK_NULL_HANDLE;

			vkDestroyFence(c_device.GetDevice(), inFlightFences[i], nullptr);
			inFlightFences[i] = VK_NULL_HANDLE;

			vkDestroySemaphore(c_device.GetDevice(), textureUploadSemaphores[i].graphicsSubmitSemaphore, nullptr);
			textureUploadSemaphores[i].graphicsSubmitSemaphore = VK_NULL_HANDLE;

			vkDestroySemaphore(c_device.GetDevice(), textureUploadSemaphores[i].transferSubmitSemaphore, nullptr);
			textureUploadSemaphores[i].transferSubmitSemaphore = VK_NULL_HANDLE;
		}

		CreateSynchronizationPrimitives();
	}

	void RendererBase::WaitForDevice() const
	{
		if (c_device.GetDevice() != VK_NULL_HANDLE)
		{
			VK_CHECK_RESULT(vkDeviceWaitIdle(c_device.GetDevice()));
		}
	}

	bool RendererBase::PrepareFrame()
	{
		if (c_window.IsPrepared() == false)
		{
			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(c_device.GetGPU(),
				c_window.Surface(), &surfaceCapabilities));

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

		VK_CHECK_RESULT(vkWaitForFences(c_device.GetDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(c_device.GetDevice(), 1, &inFlightFences[currentFrame]));

		if (m_settings.UIDisplay)
		{
			if (m_settings.hotReloadRequested == true)
			{
				pipelineManager.HotReloadShaders();

				m_settings.hotReloadRequested = false;
			}

			UserInterface::Prepare();

			VkExtent2D windowExtent = c_window.Extents();

			ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowExtent.width) / 3,
			static_cast<float>(windowExtent.height) / 2 ));

			ImGui::SetNextWindowPos(ImVec2(0,0));

			int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

			if (UserInterface::IsToggled() == false)
			{
				flags |= ImGuiWindowFlags_NoInputs;
			}

			if (ImGui::Begin("CWKVulkanEngine", nullptr, flags) == true)
			{
				UserInterface::TextData("FPS: %d", static_cast<int>(app.GetTimer().GetFPS()));
				UpdateUI();
			}

			ImGui::End();
		}

		VkResult result = 
			vkAcquireNextImageKHR(c_device.GetDevice(), swapChain.GetHandle(), UINT64_MAX,
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
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error("vkAcquireNextImageKHR failed\n");
			}
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

		bool textureSubmitted = c_textureManager.UploadTextureDataToGPU(currentFrame, textureUploadSemaphores[currentFrame]);
		if (textureSubmitted == true)
		{
			pipelineWaitStages.push_back(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			waitSemaphores.push_back(textureUploadSemaphores[currentFrame].graphicsSubmitSemaphore);
		}

		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = pipelineWaitStages.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentImageIndex];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &this->commandBuffers[currentFrame];

		VK_CHECK_RESULT(vkQueueSubmit(c_device.GetQueue(DeviceQueue::GRAPHICS).handle, 1, &submitInfo,
			inFlightFences[currentFrame]));

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentImageIndex];
		presentInfo.pImageIndices = &currentImageIndex;
		presentInfo.swapchainCount = 1;

		auto swapChainHandle = this->swapChain.GetHandle();
		presentInfo.pSwapchains = &swapChainHandle;

		VkResult result = vkQueuePresentKHR(c_device.GetQueue(DeviceQueue::PRESENT).handle, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR) 
			{
				ResizeWindow();
			}
		}
		else
		{
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error("vkQueuePresentKHR failed\n");
			}
		}

		currentFrame = (currentFrame + 1) % m_settings.maxFramesInFlight;
	}

}