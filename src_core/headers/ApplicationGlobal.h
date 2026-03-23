#pragma once


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
#define _GraphicsContext appManager.GetApplication()->GetVulkanContext()
#define _Timer appManager.GetApplication()->GetTimer()