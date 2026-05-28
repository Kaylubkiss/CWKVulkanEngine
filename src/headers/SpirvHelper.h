#pragma once

#include "shaderc/shaderc.hpp"
#include "vkUtil.h"

namespace vk::spirv
{
	struct CompilationInfo
	{
		const char* filename = nullptr;

		shaderc_shader_kind kind;

		std::string source;

		shaderc::CompileOptions options;

	};

	inline std::optional<std::vector<uint32_t>> SourceToSpv( const CompilationInfo& info )
	{
		shaderc::Compiler compiler;

		shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(info.source, info.kind, info.filename, info.options);

		if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << "[ERROR] Could not compile the shader: " + result.GetErrorMessage() << '\n';
			return std::nullopt;
		}


		const uint32_t* src = result.begin();
		size_t wordCount = result.end() - src;
		size_t sizeOfSource = wordCount * sizeof(uint32_t);

		std::vector<uint32_t> output(sizeOfSource);

		memcpy(output.data(), src, sizeOfSource);

		std::cout << "-----Shader SPIRV---\n";
		std::cout << "Magic number: " << output[0] << '\n';

		return output;
	}

	inline void WriteSpirvFile( const char* filename, const std::vector<uint32_t>& data )
	{
		std::ofstream output(filename,std::ios::out | std::ios::binary);


		if (output.is_open() == false)
		{
			std::filesystem::path filePath = filename;

			auto parentPath = filePath.parent_path();

			if (parentPath == "shaders/spirv")
			{
				if (std::filesystem::exists(parentPath) == false)
				{
					std::filesystem::create_directories(filePath.parent_path());
					output.open(filePath.string().c_str(), std::ios::out | std::ios::binary);
				}
			}

			if (output.is_open() == false)
			{
				std::cerr << "could not write to file: " + std::string(filename) << "\n";
				throw std::runtime_error("WriteSpirvFile() Failed!\n");
			}
		}

		output.write(reinterpret_cast<const char*>(data.data()), data.size());

		output.close();

		if (output.fail())
		{
			std::cerr << "could not write to file: " + std::string(filename) << "\n";
			throw std::runtime_error("WriteSpirvFile() Failed!\n");
		}
	}

	//returns the filepath to the already written spirv.
	inline std::string ConvertToSpirvFilePath(std::filesystem::path sourceFilePath,
		shaderc_shader_kind shader_kind)
	{
		if (shader_kind == shaderc_vertex_shader)
		{
			sourceFilePath.replace_filename(sourceFilePath.stem().string() + "-vert");
		}
		else if (shader_kind == shaderc_fragment_shader)
		{
			sourceFilePath.replace_filename(sourceFilePath.stem().string() + "-frag");
		}
		else if (shader_kind == shaderc_geometry_shader)
		{
			sourceFilePath.replace_filename(sourceFilePath.stem().string() + "-geom");
		}
		else if (shader_kind == shaderc_compute_shader)
		{
			sourceFilePath.replace_filename(sourceFilePath.stem().string() + "-comp");
		}
		else
		{
			std::cerr << "[ERROR] unsupported shader type: " << shader_kind << '\n';
			std::cerr << "vk::spirv::ConvertToSpirvFilePath() Failed\n";
			return "";
		}

		sourceFilePath.replace_extension("spv");

		//NOTE: this is a very strange way to verify the path of the file while specifying it.
		//Will need to move this elsewhere for functional clarity. Also don't like the hardcoding of "spirv"
		//and "shaders" (TODO)
		std::filesystem::path shader_root_path = sourceFilePath.parent_path();
		while (shader_root_path != "shaders")
		{
			if (shader_root_path.has_parent_path() == false)
			{
				throw std::runtime_error("couldn't find path: 'shaders/' !");
			}

			shader_root_path = shader_root_path.parent_path();
		}

		sourceFilePath = shader_root_path.string() + std::string("/spirv/") + sourceFilePath.filename().string();

		return sourceFilePath.string();
	}

	inline std::optional<std::string> ReadSourceAndWriteToSpirv( std::string_view sourceFilePath,
		shaderc_shader_kind shader_kind, bool forceCompilation = false)
	{
		//check if the spirv file has already been written --> reading and writing is a very slow operation.
		std::string spirvFilePath = ConvertToSpirvFilePath(sourceFilePath, shader_kind);

		if (forceCompilation == false && std::filesystem::exists(spirvFilePath) == true)
		{
			return spirvFilePath;
		}

		//vertex shader reading and compilation
		CompilationInfo shaderInfo = {};

		shaderInfo.source = vk::util::ReadFile(std::string(sourceFilePath)).value_or("");

		if (shaderInfo.source.empty())
		{
			std::cerr << "[ERROR] Couldn't successfully read shader file " << sourceFilePath << '\n';
			return std::nullopt;
		}

		shaderInfo.filename = sourceFilePath.data();
		shaderInfo.kind = shader_kind;

		auto output = SourceToSpv(shaderInfo);

		if (output.has_value() == false)
		{
			std::cerr << "[ERROR] Couldn't successfully read shader file " << sourceFilePath << '\n';
			return std::nullopt;
		}


		WriteSpirvFile(spirvFilePath.c_str(), output.value());

		return spirvFilePath;
	}

}

