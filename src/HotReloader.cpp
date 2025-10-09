/*
	* filename: HotReloader.cpp
	* author: Caleb Kissinger
*/
#include "HotReloader.h"
#include "vkUtility.h"
#include <iostream>
#include "vkInit.h"
#include "ApplicationGlobal.h"

namespace vk 
{

	/*
		* @brief private helper method which adds a pipeline the hot reloader 
		can edit if changes occur in its associated shader modules.
		* @param pipeline: the pipeline which has the shader stages associated.
	*/
	/*void HotReloader::AddPipeline(vk::Pipeline& pipeline) 
	{

		this->pipelinePtr = (&pipeline);

		const std::vector<vk::ShaderModuleInfo>& shaderModules = pipeline.ShaderModules();
		

		for (size_t i = 0; i < shaderModules.size(); ++i)
		{
			struct stat fileStat;
			
			if (stat(shaderModules[i].mFilePath.c_str(), &fileStat) != 0)
			{
				std::cerr << "[ERROR] Can't open file: " << shaderModules[i].mFilePath << '\n';
			}
			else
			{
				shaderInfos.push_back({ i, fileStat.st_mtime});
			}
		}

		
	}*/
	
	/*
		* @brief Called every frame to check the file status of a shader, and recreates
		* the pipeline's shader modules if any change occurs.
	*/
	void HotReloader::HotReload() 
	{
		//bool somethingChanged = false;
		//const VkDevice& appDevice = (this->appDevicePtr);


		//for (size_t i = 0; i < shaderInfos.size(); ++i)
		//{
		//	struct stat fileStat;
		//	
		//	vk::ShaderModuleInfo& shaderModule = pipelinePtr->ShaderModule(shaderInfos[i].module_i);

		//	std::string filePath = SHADER_PATH + shaderModule.mFileName;

		//	if (stat(filePath.c_str(), &fileStat) != 0)
		//	{
		//		std::cerr << "[ERROR] Can't Read File " << filePath << '\n';
		//		continue;
		//	}

		//	//check if the filesystem saw any changes.
		//	if (fileStat.st_mtime != shaderInfos[i].last_modification)
		//	{
		//		std::string shaderPath = 
		//			vk::util::ReadSourceAndWriteToSprv(filePath, shaderModule.mShaderKind);

		//		if (shaderPath.empty())
		//		{
		//			std::cerr << "[ERROR] Couldn't successfully write to file " << shaderModule.mFileName << '\n';
		//			continue;
		//		}

		//		somethingChanged = true;

		//		VK_CHECK_RESULT(vkDeviceWaitIdle(appDevicePtr))

		//		vkDestroyShaderModule(appDevice, shaderModule.mHandle, nullptr);
		//		shaderModule.mHandle = vk::init::ShaderModule(appDevice, shaderPath.data());

		//		shaderInfos[i].last_modification = fileStat.st_mtime;
		//	}
		//}

		//if (somethingChanged) 
		//{
		//	pipelinePtr->Recreate(appDevice);
		//}
	}

}