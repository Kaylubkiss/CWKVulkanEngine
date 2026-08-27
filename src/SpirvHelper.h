#ifndef SPIRV_HELPER_HPP
#define SPIRV_HELPER_HPP

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

	inline std::vector<uint32_t> SourceToSpv( const CompilationInfo& info )
	{
		static shaderc::Compiler compiler;

		shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(info.source, info.kind, info.filename, info.options);

		if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << "[ERROR] Could not compile the shader: " + result.GetErrorMessage() << '\n';
			return {};
		}

		std::cout << "-----Shader SPIRV---\n";
		std::cout << "Magic number: " << *result.cbegin() << '\n';

		return std::vector<uint32_t>(result.cbegin(), result.cend());
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

		output.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(uint32_t));

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
		switch (shader_kind)
		{
			case shaderc_vertex_shader:
				sourceFilePath.replace_filename(sourceFilePath.stem().string() + "-vert");
				break;
			case shaderc_fragment_shader:
				sourceFilePath.replace_filename(sourceFilePath.stem().string() + "-frag");
				break;
			case shaderc_geometry_shader:
				sourceFilePath.replace_filename(sourceFilePath.stem().string() + "-geom");
				break;
			case shaderc_compute_shader:
				sourceFilePath.replace_filename(sourceFilePath.stem().string() + "-comp");
				break;
			default:
				std::cerr << "[ ERROR ] unsupported shader type: " << shader_kind << '\n';
				std::cerr << "vk::spirv::ConvertToSpirvFilePath() Failed\n";
				return "";
		}

		sourceFilePath.replace_extension("spv");

		//NOTE: this is a very strange way to verify the path of the file while specifying it.
		//Will need to move this elsewhere for functional clarity. Also don't like the hardcoding of "spirv"
		//and "shaders" (TODO)
		std::filesystem::path shader_root_path = sourceFilePath.parent_path();
		while (shader_root_path != "shaders" && shader_root_path != shader_root_path.parent_path())
		{
			shader_root_path = shader_root_path.parent_path();
		}

		if (shader_root_path != "shaders")
		{
			return "";
		}

		sourceFilePath = shader_root_path.string() + std::string("/spirv/") + sourceFilePath.filename().string();

		return sourceFilePath.string();
	}

	inline std::string ReadSourceAndWriteToSpirv( std::string_view sourceFilePath,
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

		shaderInfo.filename = sourceFilePath.data();
		shaderInfo.kind = shader_kind;

		shaderInfo.source = vk::util::ReadFile(std::string(sourceFilePath));
		if (shaderInfo.source.empty())
		{
			std::cerr << "[ERROR] Couldn't successfully read shader file " << sourceFilePath << '\n';
			return {};
		}



		std::vector<uint32_t> output = SourceToSpv(shaderInfo);

		if (output.empty())
		{
			std::cerr << "[ERROR] Couldn't successfully read shader file " << sourceFilePath << '\n';
			return {};
		}


		WriteSpirvFile(spirvFilePath.c_str(), output);

		return spirvFilePath;
	}

}

#endif

