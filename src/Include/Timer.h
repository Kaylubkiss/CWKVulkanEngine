#pragma once
#include <SDL2/SDL.h>

class Time 
{
	uint64_t timeNow = 0;
	uint64_t timeBefore = 0;
	double deltaTime = 0.f;
	
public:
	Time() = default;
	Time(uint64_t currentTime);
	double DeltaTime() const;
	void Update();

};