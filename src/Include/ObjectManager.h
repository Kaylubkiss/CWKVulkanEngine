#pragma once


#include "Object.h"
#include <map>
#include <thread>
#include "Threadpool.h"

static std::string PathToObjects() {

	return "External/objects/";
}

struct str_cmp 
{
	bool operator()(const char* a, char const* b) const 
	{
		return std::strcmp(a, b) < 0;
	}

};
class ObjectManager 
{

public:
	ObjectManager();
	~ObjectManager() 
	{
		objects.clear();
	}

	void LoadObject(const char* name = nullptr, const char* filename = nullptr, bool willDebugDraw = false, const glm::mat4& modelTransform = glm::mat4(1.f));

	Object& operator[](const char* name) 
	{
		size_t count = objects.count(name);

		if (objects.count(name) > 0)
		{
			return *objects[name];
		}
		else 
		{
			throw std::runtime_error("no object in object manager!\n");
		}
	}

	ThreadPool mThreadWorkers;

private:
	void LoadObjParallel(const char* name = nullptr, const char* filename = nullptr, bool willDebugDraw = false, const glm::mat4& modelTransform = glm::mat4(1.f));

	std::map<const char*, Object*, str_cmp> objects;
	std::mutex map_mutex;


};
