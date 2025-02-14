#include "ObjectManager.h"
#include "ThreadPool.h"

void ObjectManager::LoadObjParallel(const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
{
	objects[name] = new Object(filename, willDebugDraw, modelTransform);
}

void ObjectManager::LoadObject(const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
{
	std::function<void()> func = [this, modelTransform, name, filename, willDebugDraw] { ObjectManager::LoadObjParallel(name, (PathToObjects() + std::string(filename)).c_str(), willDebugDraw, modelTransform); };

	mThreadWorkers.EnqueueTask(func);
	
}


ObjectManager::ObjectManager() 
{
	//keep it to one thread.
	this->mThreadWorkers.Init(1);
}