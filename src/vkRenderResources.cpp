#include "vkRenderResources.h"
#include "ApplicationGlobal.h"
#include "vkInit.h"

namespace vk 
{

	static const std::string shaderPath{ "Shaders/" };

	void RenderResources::Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow)
	{
		assert(_Application != NULL);

		Camera& appCamera = _Application->GetCamera();

		this->depthInfo = vk::init::CreateDepthResources(p_device, l_device, appWindow.viewport);

		this->renderPass = vk::init::RenderPass(l_device, depthInfo.depthFormat);

		this->commandPool = vk::init::CommandPool(l_device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		this->commandBuffer = vk::init::CommandBuffer(l_device, this->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		//uniform transform for objects of default pipeline.
		this->uTransform = {
		glm::mat4(1.f), //model
		appCamera.LookAt(), //view
		glm::perspective(glm::radians(45.f), (float)appWindow.viewport.width / appWindow.viewport.height, 0.1f, 1000.f) //proj
		};

		this->uTransform.proj[1][1] *= -1.f;

		//uniform(s)
		this->uniformBuffer = vk::Buffer(p_device, l_device, sizeof(uTransformObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)&this->uTransform);

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


	void RenderResources::Destroy(const VkDevice l_device)
	{
		//pipeline info...
		vkDestroyDescriptorSetLayout(l_device, this->defaultDescriptorSetLayout, nullptr);
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
}
