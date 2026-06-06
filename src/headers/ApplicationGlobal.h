#ifndef APPLICATION_GLOBAL_HPP
#define APPLICATION_GLOBAL_HPP

#include "Application.h"

class ApplicationManager
{
public:
	ApplicationManager();
	~ApplicationManager();
	Application* GetApplication();
private:
	std::unique_ptr<Application> mApp = nullptr;
};

extern ApplicationManager appManager;

#define _Application appManager.GetApplication()
#define _GraphicsContext appManager.GetApplication()->GetVulkanRenderer()
#define _Timer appManager.GetApplication()->GetTimer()

#endif