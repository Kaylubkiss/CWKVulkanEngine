#pragma once


#include "shaderc/shaderc.hpp"
#include <sys/stat.h>

namespace vk 
{
	class HotReloader
	{
		struct ShaderPathInfo
		{
			const char* path = nullptr;
			
			time_t last_modification;
		};


		std::vector<ShaderPathInfo> filepaths;

		public:
			void Add(const char* filepath);
			void HotReload();
	};


}