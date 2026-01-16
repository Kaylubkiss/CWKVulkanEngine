#include "ApplicationGlobal.h"

static int s_count = 0;

ApplicationManager appManager;

ApplicationManager::ApplicationManager()
{
	if (++s_count == 1)
	{
		mApp = std::make_unique<Application>();
	}
}

ApplicationManager::~ApplicationManager()
{
	--s_count;
}


Application* ApplicationManager::GetApplication()
{
	return mApp.get();
}