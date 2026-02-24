#include "Timer.h"


Timer::Timer()
{
	timeNow = SDL_GetPerformanceCounter();
}

double Timer::CalculateDeltaTime()
{
	this->timeBefore = this->timeNow;
	this->timeNow = SDL_GetPerformanceCounter();

	return ((this->timeNow - this->timeBefore)) / static_cast<double>(SDL_GetPerformanceFrequency());
}