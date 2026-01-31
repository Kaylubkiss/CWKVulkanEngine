#pragma once

#include "Application.h"

struct ApplicationManager
{
	ApplicationManager();
	~ApplicationManager();
	Application* GetApplication();

private:
	std::unique_ptr<Application> mApp = nullptr;
};

extern ApplicationManager appManager;

#define _Application appManager.GetApplication()
#define _GraphicsContext appManager.GetApplication()->Context()
#define _ObjectManager appManager.GetApplication()->GetObjectManager()