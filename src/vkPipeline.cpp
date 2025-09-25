/*
	* filename: VkPipeline.cpp
	* author: Caleb Kissinger
*/
#include "vkPipeline.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <iostream>

namespace vk 
{

	ShaderModuleInfo::ShaderModuleInfo(const VkDevice l_device, std::string filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shaderc_kind) : mFileName(filename), mFlags(shaderFlags), mShaderKind(shaderc_kind)
	{

		std::string shaderPath = vk::util::ReadSourceAndWriteToSprv(SHADER_PATH + filename, shaderc_kind);

		mFilePath = shaderPath;

		if (shaderPath.empty())
		{
			std::cerr << "[ERROR] Couldn't successfully read shader file " << filename << '\n';
			return;
		}

		this->mHandle = vk::init::ShaderModule(l_device, shaderPath.data());
	}

	vk::Pipeline& Pipeline::AddRenderPass(const VkRenderPass& renderPass) 
	{
		this->mRenderPass = renderPass;

		return *this;
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

	vk::Pipeline& Pipeline::AddPipelineLayout(const VkPipelineLayout& pipelineLayout) 
	{
		this->layout = pipelineLayout;

		return *this;
	}

	vk::Pipeline& Pipeline::Finalize(const VkDevice l_device, const VkPhysicalDevice p_device, const vk::Window& appWindow, VkPrimitiveTopology topology)
	{
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

		vkDestroyPipelineLayout(l_device, this->layout, nullptr);
		vkDestroyPipeline(l_device, this->handle, nullptr);


		for (size_t i = 0; i < shaderModules.size(); ++i) {
			vkDestroyShaderModule(l_device, shaderModules[i].mHandle, nullptr);
		}

	}

	const VkPipeline Pipeline::Handle() const {
		return this->handle;
	}

	const VkPipelineLayout Pipeline::Layout() const {
		return this->layout;
	}

	ShaderModuleInfo& Pipeline::ShaderModule(size_t index)
	{
		if (index < 0 || index >= shaderModules.size()) 
		{
			std::cerr << "could not index into pipeline's shader modules!\n";
		}

		return this->shaderModules[index];
	}

	const std::vector<vk::ShaderModuleInfo>& Pipeline::ShaderModules() const 
	{
		return this->shaderModules;
	}
	 
	VkRenderPass Pipeline::RenderPass()
	{
		return mRenderPass;
	}



}