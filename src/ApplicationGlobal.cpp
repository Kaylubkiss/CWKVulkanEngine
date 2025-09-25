#include "ApplicationGlobal.h"

static int s_count = 0;

ApplicationManager appManager;

ApplicationManager::ApplicationManager()
{
	if (++s_count == 1)
	{
		mApp = new Application();
	}
}

ApplicationManager::~ApplicationManager()
{
	if (--s_count == 0)
	{
		delete mApp;
	}
}


Application* ApplicationManager::GetApplication()
{
	return mApp;
}