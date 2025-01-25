#include <iostream>
#include "Application.h"
#include <filesystem>

static void Locatecwd()
{
    auto cwd = std::filesystem::current_path();
    while (cwd.has_parent_path() && cwd.parent_path() != cwd)
    {
        if (std::filesystem::exists(cwd.string() + "/FrameWork/"))
        {
            std::filesystem::current_path(cwd/"FrameWork");
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