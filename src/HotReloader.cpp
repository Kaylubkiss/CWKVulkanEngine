
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
			if (stat(shaderModules[i].filepath.c_str(), &fileStat[i]) != 0)
			{
				std::cerr << "[ERROR] Can't open file: " << shaderModules[i].filepath.c_str() << '\n';
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
	/*	return;*/
		for (size_t i = 0; i < shaderInfos.size(); ++i)
		{
			struct stat fileStat;
			
			std::vector<vk::ShaderModuleInfo>& shaderModuleVector = pipelinePtr->ShaderModules();
			vk::ShaderModuleInfo& shaderModule = shaderModuleVector[shaderInfos[i].module_i];

			
			std::string filePath = shaderModule.filepath;

			////TODO: rename the file according to if it was a vert or frag file
			////WARNING: sloow!!!
			//for (size_t i = 0; i < filePath.size(); ++i)
			//{
			//	if (filePath[i] == '.')
			//	{
			//		size_t ext_size = filePath.size() - i;

			//		filePath.resize(filePath.size() - ext_size);

			//		break;
			//	}
			//}

			if (stat(filePath.c_str(), &fileStat) != 0)
			{
				std::cerr << "[ERROR] Can't Read File " << filePath << '\n';
				continue;
			}

			if (fileStat.st_mtime != shaderInfos[i].last_modification)
			{
				std::string shaderPath = vk::util::ReadSourceAndWriteToSprv(
					filePath, shaderModule.shaderc_kind
				);

				if (shaderPath.empty())
				{
					std::cerr << "[ERROR] Couldn't successfully write to file " << shaderModule.filepath << '\n';
					continue;
				}

				somethingChanged = true;

				vkDestroyShaderModule(appDevice, shaderModule.handle, nullptr);
				shaderModule.handle = vk::init::ShaderModule(appDevice, shaderPath.data());
			}
		}


		if (somethingChanged) 
		{
			pipelinePtr->Recreate(appDevice, this->renderPassPtr);
		}
	}


	HotReloader& HotReloader::operator=(const HotReloader& other)
	{
		if (this == &other) 
		{
			return *this;
		}

		appDevicePtr = other.appDevicePtr;
		renderPassPtr = other.renderPassPtr;
		shaderInfos = other.shaderInfos;

		pipelinePtr = other.pipelinePtr;

		return *this;
	}
}