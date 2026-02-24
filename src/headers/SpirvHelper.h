#pragma once

#include "shaderc/shaderc.hpp"


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

		if (!output.is_open())
		{
			std::cerr << "could not write to file: " + std::string(filename) << "\n";
			throw std::runtime_error("WriteSpirvFile() Failed!\n");
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
		else
		{
			std::cerr << "[ERROR] unsupported shader type: " << shader_kind << '\n';
			return "";
		}

		sourceFilePath.replace_extension("spv");

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

