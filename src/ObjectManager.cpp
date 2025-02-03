#include "ObjectManager.h"


void ObjectManager::LoadObject(const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
{
	objects[name] = new Object(filename, willDebugDraw, modelTransform);
}