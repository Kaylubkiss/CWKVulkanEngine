#include "Timer.h"


Timer::Timer(uint64_t currentTime) : timeNow(currentTime), timeBefore(), deltaTime() {}

double Timer::CalculateDeltaTime()
{
	this->timeBefore = this->timeNow;
	this->timeNow = SDL_GetPerformanceCounter();

	return ((this->timeNow - this->timeBefore)) / (double)SDL_GetPerformanceFrequency();
}