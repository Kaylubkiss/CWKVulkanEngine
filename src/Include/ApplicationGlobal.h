#pragma once
#include "Application.h"

struct ApplicationManager
{
	ApplicationManager();
	~ApplicationManager();
	Application* GetApplication();

private:
	Application* mApp = nullptr;
};

extern ApplicationManager appManager;

#define _Application appManager.GetApplication()