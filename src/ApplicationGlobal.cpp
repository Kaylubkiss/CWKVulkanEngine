#include "ApplicationGlobal.h"

static int s_count = 0;
//static typename std::aligned_storage<sizeof(Application), alignof(Application)>::type applicationBuffer;
//
//Application& app = reinterpret_cast<Application&> (applicationBuffer);

ApplicationManager appManager;

ApplicationManager::ApplicationManager()
{
	if (++s_count == 1)
	{
		/*std::cout << "here\n";
		std::cout << sizeof(applicationBuffer) << '\n';*/
		mApp = new Application();
	}
}

ApplicationManager::~ApplicationManager()
{
	if (--s_count == 0)
	{
		mApp->~Application();
	}
}


Application* ApplicationManager::GetApplication()
{
	return mApp;
}