#pragma once
#include "vkBuffer.h"


enum LightCountIndex {
	DIR_IND = 0,
	POS_IND,
	MAX_IND_COUNT
};

static const int MaxLights = 1024;


//WARNING: THIS STRUCT IS 33kb!!!
struct LightInfoObject
{
	int num_lights = 0;
	glm::vec3 lightPos[MaxLights] = { {0,0,0} };
	int curr_index[MAX_IND_COUNT] = { 0 };
	bool isUpdated = false;
	vk::Buffer mBuffer;

private:
	void AddPosition(const glm::vec3& pos);

public:
	void Deallocate();
	void Create(const glm::vec3& pos, const glm::vec3& dir);
	void Update();
};
