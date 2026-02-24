#pragma once

class Timer 
{
public:
	Timer();
	double CalculateDeltaTime();
private:
	uint64_t timeNow    = 0;
	uint64_t timeBefore = 0;
};