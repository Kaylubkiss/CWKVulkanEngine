#pragma once
#include "Application.h"

static struct ApplicationManager
{
	ApplicationManager();
	~ApplicationManager();
	Application* GetApplication();
} appManager;


#define _Application appManager.GetApplication()
#define _Window appManager.GetApplication()->GetWindowInfo()