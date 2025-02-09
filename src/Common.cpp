#include "Application.h"

static int s_count = 0;
static typename std::aligned_storage<sizeof(Application), alignof(Application)>::type applicationBuffer;

Application& app = reinterpret_cast<Application&> (applicationBuffer);


ApplicationManager::ApplicationManager()
{
	if (++s_count == 1)
	{
		new (&app) Application();
	}
}

ApplicationManager::~ApplicationManager()
{
	if (--s_count == 0)
	{
		(&app)->~Application();
	}
}


Application* ApplicationManager::GetApplication()
{
	return (&app);
}







