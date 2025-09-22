#include "UserInterface.h"
#include "vkInit.h"

namespace vk {

	UserInterface::UserInterface() 
	{
		

	}

	UserInterface::~UserInterface() 
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

	}

}