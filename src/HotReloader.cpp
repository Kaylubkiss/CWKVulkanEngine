
#include "HotReloader.h"
#include "vkUtility.h"
#include <iostream>
#include "vkInit.h"
#include "ApplicationGlobal.h"

#define NUM_MODULES 5

namespace vk 
{
#ifdef _DEBUG
	HotReloader::HotReloader(VkDevice* l_device, vk::Pipeline* pipeline, VkRenderPass* renderPass)
	{
		if (l_device == nullptr) 
		{
			throw std::runtime_error("[ERROR] Passed in invalid device to hot reloader\n");
		}

		if (renderPass == nullptr) {

			throw std::runtime_error("[ERROR] Passed in invalid renderpass to hot reloader\n");
		}

		appDevicePtr = l_device; 
		renderPassPtr = renderPass;

		AddPipeline(pipeline);
	}

	void HotReloader::AddPipeline(vk::Pipeline* pipeline) 
	{
		if (pipeline == nullptr) 
		{
			std::cerr << "[ERROR] Hot reloader could not add NULL pipeline\n";
			return;
		}

		this->pipelinePtr = pipeline;

		struct stat fileStat[NUM_MODULES];

		std::vector<vk::ShaderModuleInfo>& shaderModules = pipeline->ShaderModules();

		if (NUM_MODULES < shaderModules.size())
		{
			throw std::runtime_error(
				"[ERROR] Could overrun array in hot reloader Add() function I should probably fix that.\n"
			);
		}

		for (size_t i = 0; i < shaderModules.size(); ++i)
		{
			if (stat(shaderModules[i].filepath.c_str(), &fileStat[i]) != 0)
			{
				std::cerr << "[ERROR] Can't open file: " << shaderModules[i].filepath.c_str() << '\n';
			}
			else
			{
				shaderInfos.push_back({ &shaderModules[i], fileStat[i].st_mtime });
			}
		}

		
	}


	void HotReloader::HotReload() 
	{
		bool somethingChanged = false;
		const VkDevice& appDevice = *(this->appDevicePtr);

		for (size_t i = 0; i < shaderInfos.size(); ++i)
		{
			struct stat fileStat;
			const char* filePath = shaderInfos[i].moduleInfo->filepath.data();

			if (stat(filePath, &fileStat) != 0)
			{
				std::cerr << "[ERROR] Can't Read File " << filePath << '\n';
				continue;
			}

			if (fileStat.st_mtime != shaderInfos[i].last_modification)
			{
				std::string shaderPath = vk::util::ReadSourceAndWriteToSprv(
					filePath, shaderInfos[i].moduleInfo->shaderc_kind
				);

				if (shaderPath.empty())
				{
					std::cerr << "[ERROR] Couldn't successfully write to file " << shaderInfos[i].moduleInfo->filepath << '\n';
					continue;
				}

				somethingChanged = true;

				vkDestroyShaderModule(appDevice, shaderInfos[i].moduleInfo->handle, nullptr);
				shaderInfos[i].moduleInfo->handle = vk::init::ShaderModule(appDevice, shaderPath.data());
			}
		}


		if (somethingChanged) 
		{
			this->pipelinePtr->Recreate(appDevice, *this->renderPassPtr);
		}


	}
#endif
}