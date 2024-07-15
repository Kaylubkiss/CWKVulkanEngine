#pragma once
#include <SDL2/SDL.h>

class Time 
{
	uint64_t timeNow;
	uint64_t timeBefore;
	double deltaTime;
	
public:
	Time() = default;
	Time(uint64_t currentTime);
	double DeltaTime() const;
	void Update();

};