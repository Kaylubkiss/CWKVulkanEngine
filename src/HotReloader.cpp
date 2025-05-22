
#include "HotReloader.h"
#include "vkUtility.h"
#include <iostream>
#include "vkInit.h"
#include "ApplicationGlobal.h"

#define NUM_MODULES 5

namespace vk 
{
	
	HotReloader::HotReloader(VkDevice* l_device, vk::Pipeline& pipeline, VkRenderPass* renderPass)
	{
		if (l_device == nullptr) 
		{
			throw std::runtime_error("[ERROR] Passed in invalid device to hot reloader\n");
		}

		if (renderPass == nullptr) {

			throw std::runtime_error("[ERROR] Passed in invalid renderpass to hot reloader\n");
		}

		appDevicePtr = *l_device; 
		renderPassPtr = *renderPass;

		AddPipeline(pipeline);
	}

	
	void HotReloader::AddPipeline(vk::Pipeline& pipeline) 
	{

		this->pipelinePtr = (&pipeline);

		struct stat fileStat[NUM_MODULES];

		std::vector<vk::ShaderModuleInfo>& shaderModules = pipeline.ShaderModules();

		
		if (NUM_MODULES < shaderModules.size())
		{
			throw std::runtime_error(
				"[ERROR] Could overrun array in hot reloader Add() function I should probably fix that.\n"
			);
		}

		for (size_t i = 0; i < shaderModules.size(); ++i)
		{
			std::string filePath = SHADER_PATH + shaderModules[i].filename;

			if (stat(filePath.c_str(), &fileStat[i]) != 0)
			{
				std::cerr << "[ERROR] Can't open file: " << filePath.c_str() << '\n';
			}
			else
			{
				shaderInfos.push_back({ i, fileStat[i].st_mtime});
			}
		}

		
	}
	
	void HotReloader::HotReload() 
	{
		bool somethingChanged = false;
		const VkDevice& appDevice = (this->appDevicePtr);


		for (size_t i = 0; i < shaderInfos.size(); ++i)
		{
			struct stat fileStat;
			
			std::vector<vk::ShaderModuleInfo>& shaderModuleVector = pipelinePtr->ShaderModules();
			vk::ShaderModuleInfo& shaderModule = shaderModuleVector[shaderInfos[i].module_i];
			std::string filePath = SHADER_PATH + shaderModule.filename;

			if (stat(filePath.c_str(), &fileStat) != 0)
			{
				std::cerr << "[ERROR] Can't Read File " << filePath << '\n';
				continue;
			}

			if (fileStat.st_mtime != shaderInfos[i].last_modification)
			{
				std::string shaderPath = 
					vk::util::ReadSourceAndWriteToSprv(filePath, shaderModule.shaderc_kind);

				if (shaderPath.empty())
				{
					std::cerr << "[ERROR] Couldn't successfully write to file " << shaderModule.filename << '\n';
					continue;
				}

				somethingChanged = true;

				VK_CHECK_RESULT(vkDeviceWaitIdle(appDevicePtr))

				vkDestroyShaderModule(appDevice, shaderModule.handle, nullptr);
				shaderModule.handle = vk::init::ShaderModule(appDevice, shaderPath.data());

				shaderInfos[i].last_modification = fileStat.st_mtime;
			}
		}


		if (somethingChanged) 
		{
			pipelinePtr->Recreate(appDevice, this->renderPassPtr);
		}
	}

}