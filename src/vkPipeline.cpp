/*
	* filename: VkPipeline.cpp
	* author: Caleb Kissinger
*/
#include "vkPipeline.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "vkResource.h"
#include <iostream>

namespace vk 
{

	ShaderModuleInfo::ShaderModuleInfo(const VkDevice l_device, std::string filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shaderc_kind) : mFilename(filename), mFlags(shaderFlags), mShaderKind(shaderc_kind)
	{

		std::string shaderPath = vk::util::ReadSourceAndWriteToSprv(SHADER_PATH + filename, shaderc_kind);

		if (shaderPath.empty())
		{
			std::cerr << "[ERROR] Couldn't successfully read shader file " << filename << '\n';
			return;
		}

		this->mHandle = vk::init::ShaderModule(l_device, shaderPath.data());
	}


	Pipeline& Pipeline::AddModule(const ShaderModuleInfo& shaderModuleInfo)
	{
		shaderModules.push_back(shaderModuleInfo);

		return *this;
	}

	void Pipeline::Recreate(const VkDevice l_device) 
	{
		//NOTE: globals, i.e. descriptor set layout 
		// cannot be edited in the shader during runtime!!
		vkDestroyPipeline(l_device, this->handle, nullptr);


		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

		for (size_t i = 0; i < shaderModules.size(); ++i) {
			shaderStageCreateInfo.push_back(vk::init::PipelineShaderStageCreateInfo(shaderModules[i].mHandle, shaderModules[i].mFlags));
		}

		this->handle = vk::init::CreateGraphicsPipeline(l_device, this->layout, this->mRenderPass,
			shaderStageCreateInfo.data(), shaderStageCreateInfo.size(),
			this->mTopology);
	}

	vk::Pipeline& Pipeline::Finalize(const VkDevice l_device, const VkPhysicalDevice p_device, const vk::Window& appWindow, VkPrimitiveTopology topology)
	{
		this->mRenderDepthInfo = vk::rsc::CreateDepthResources(p_device, l_device, appWindow.viewport);

		this->mRenderPass = vk::init::RenderPass(l_device, this->mRenderDepthInfo.depthFormat);


		this->descriptorSetLayout = vk::init::DescriptorSetLayout(l_device);

		this->layout = vk::init::CreatePipelineLayout(l_device, this->descriptorSetLayout);

		this->mTopology = topology;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

		for (size_t i = 0; i < shaderModules.size(); ++i){
			shaderStageCreateInfo.push_back(vk::init::PipelineShaderStageCreateInfo(shaderModules[i].mHandle, shaderModules[i].mFlags));
		}

		this->handle = vk::init::CreateGraphicsPipeline(l_device, this->layout, this->mRenderPass,
												shaderStageCreateInfo.data(), 
												shaderStageCreateInfo.size(), 
												this->mTopology);

		return *this;
	}

	void Pipeline::Destroy(const VkDevice l_device) 
	{
		vkDestroyRenderPass(l_device, this->mRenderPass, nullptr);

		this->mRenderDepthInfo.Destroy(l_device);

		vkDestroyDescriptorSetLayout(l_device, this->descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(l_device, this->layout, nullptr);
		vkDestroyPipeline(l_device, this->handle, nullptr);


		for (size_t i = 0; i < shaderModules.size(); ++i) {
			vkDestroyShaderModule(l_device, shaderModules[i].mHandle, nullptr);
		}

	}

	const VkDescriptorSetLayout Pipeline::DescriptorSetLayout() const {
		return this->descriptorSetLayout;
	}

	const VkPipeline Pipeline::Handle() const {
		return this->handle;
	}

	const VkPipelineLayout Pipeline::Layout() const {
		return this->layout;
	}

	std::vector<ShaderModuleInfo>& Pipeline::ShaderModules() 
	{
		return this->shaderModules;
	}

	VkRenderPass Pipeline::RenderPass()
	{
		return mRenderPass;
	}

	vk::rsc::DepthResources& Pipeline::RenderDepthInfo() 
	{
		return this->mRenderDepthInfo;
	}



}