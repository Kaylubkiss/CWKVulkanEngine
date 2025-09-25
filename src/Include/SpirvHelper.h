#pragma once

#include "shaderc/shaderc.hpp"
#include <vector>
#include <stdexcept>
#include <iostream>

namespace vk 
{
	namespace shader 
	{
		struct CompilationInfo 
		{
			const char* filename = nullptr;

			shaderc_shader_kind kind;

			std::string source;

			shaderc::CompileOptions options;

		};

		inline void PreprocessShader(CompilationInfo& info)
		{
			shaderc::Compiler compiler;
			

			shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(info.source, info.kind, info.filename, info.options);

			if (result.GetCompilationStatus() != shaderc_compilation_status_success) 
			{
				throw std::runtime_error("could not preprocess the shader: " + result.GetErrorMessage());
			}

			const char* src = result.cbegin();

			size_t newSize = result.cend() - src;
			info.source.resize(newSize);
			
			memcpy(info.source.data(), src, newSize);
			
			std::cout << "-----Shader Source---\n";
			std::string output = { info.source.data(), info.source.data() + newSize };
			std::cout << output << '\n';



		}

		inline void SourceToAssembly(CompilationInfo& info) 
		{
			shaderc::Compiler compiler;

			shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(info.source, info.kind, info.filename, info.options);

			if (result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				throw std::runtime_error("could not compile the shader: " + result.GetErrorMessage());
			}


			const char* src = result.cbegin();
			size_t newSize = result.cend() - src;
			info.source.resize(newSize);

			memcpy(info.source.data(), src, newSize);

			std::cout << "-----Shader SPIRV Assembly---\n";
			std::string output = { info.source.data(), info.source.data() + newSize };
			std::cout << output << '\n';
		}

		inline std::vector<uint32_t> SourceToSpv(const CompilationInfo& info)
		{
			shaderc::Compiler compiler;

			shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(info.source, info.kind, info.filename, info.options);

			if (result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				std::cerr << "[ERROR] Could not compile the shader: " + result.GetErrorMessage() << '\n';
				return std::vector<uint32_t>();
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

	}

}