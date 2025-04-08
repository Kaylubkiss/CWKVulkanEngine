#include "vkPipeline.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <iostream>
#ifdef _DEBUG
#include "SpirvHelper.h"
#endif

namespace vk 
{
	#define SHADER_PATH "Shaders/"

	void Pipeline::AddModule(const VkDevice l_device, const std::string& filename, VkShaderStageFlagBits type, shaderc_shader_kind shader_kind)
	{
		std::string shaderPath = SHADER_PATH + filename;
		
		#ifdef _DEBUG
			std::cout << "WARNING: make sure to load .spv instead of .vert in release!\n";
			//vertex shader reading and compilation
			vk::shader::CompilationInfo shaderInfo = {};
			shaderInfo.source = vk::util::ReadFile(shaderPath);
			shaderInfo.filename = filename.c_str();
			shaderInfo.kind = shader_kind;

			std::vector<uint32_t> output = vk::shader::SourceToSpv(shaderInfo);


			//WARNING: sloow!!!
			for (size_t i = 0; i < shaderPath.size(); ++i) 
			{
				if (shaderPath[i] == '.') 
				{
					size_t ext_size = shaderPath.size() - i;

					shaderPath.resize(shaderPath.size() - ext_size);

					break;
				}
			}

			if (shader_kind == shaderc_vertex_shader) { shaderPath += "vert"; }
			else if (shader_kind == shaderc_fragment_shader) { shaderPath += "frag";}
			else {
				std::cerr << "unsupported shader type: " << shader_kind << '\n';
				return;
			}

			shaderPath += ".spv";

			vk::util::WriteSpirvFile(shaderPath.data(), output);
		#endif

		shaderModules.push_back({ vk::init::ShaderModule(l_device, shaderPath.data()), type });
	}

	void Pipeline::Finalize(const VkDevice l_device, const VkRenderPass renderPass, VkPrimitiveTopology topology) 
	{
		this->descriptorSetLayout = vk::init::DescriptorSetLayout(l_device);

		this->layout = vk::init::CreatePipelineLayout(l_device, this->descriptorSetLayout);

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

		for (size_t i = 0; i < shaderModules.size(); ++i){
			shaderStageCreateInfo.push_back(vk::init::PipelineShaderStageCreateInfo(shaderModules[i].module, shaderModules[i].flags));
		}

		this->handle = vk::init::CreatePipeline(l_device, this->layout, renderPass,
												shaderStageCreateInfo.data(), shaderStageCreateInfo.size(), 
												topology);
	}

	void Pipeline::Destroy(const VkDevice l_device) 
	{

		vkDestroyDescriptorSetLayout(l_device, this->descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(l_device, this->layout, nullptr);
		vkDestroyPipeline(l_device, this->handle, nullptr);


		for (size_t i = 0; i < shaderModules.size(); ++i) {
			vkDestroyShaderModule(l_device, shaderModules[i].module, nullptr);
		}

	}


}