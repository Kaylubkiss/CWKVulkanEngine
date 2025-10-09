#pragma once

class Timer 
{
	uint64_t timeNow = 0;
	uint64_t timeBefore = 0;
	double deltaTime = 0.f;
	
public:
	Timer() = default;
	Timer(uint64_t currentTime);
	double CalculateDeltaTime();

};