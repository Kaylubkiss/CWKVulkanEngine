#include <iostream>
#include "ApplicationGlobal.h"
#include <filesystem>

static void Locatecwd()
{
    std::filesystem::path cwd = std::filesystem::current_path();
    while (cwd.has_parent_path() && cwd.parent_path() != cwd)
    {
        if (std::filesystem::exists(cwd.string() + "/src/"))
        {
            std::filesystem::current_path(cwd/"src");
            return;
        }
        cwd = cwd.parent_path();
    }
}

int main(int argc, char* argv[])
{

    Locatecwd();

	_Application->run();

	return 0;
}