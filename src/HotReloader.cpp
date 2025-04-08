#include "HotReloader.h"
#include <iostream>

namespace vk 
{
	void HotReloader::Add(const char* filepath) 
	{
		struct stat fileStat;

		if (stat(filepath, &fileStat) == 0) 
		{
			filepaths.push_back({ filepath, fileStat.st_mtime });
		}
		else 
		{
			std::cerr << "File invalid, can't add to hot reloader: " << filepath << '\n';
		}
	}


	void HotReloader::HotReload() 
	{
		for (size_t i = 0; i < filepaths.size(); ++i) 
		{
			struct stat fileStat;

			stat(filepaths[i].path, &fileStat);

			if (fileStat.st_mtime != filepaths[i].last_modification) 
			{


			}

		}

	}

}