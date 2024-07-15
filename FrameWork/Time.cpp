#include "Time.h"

double Time::DeltaTime() const
{
	return this->deltaTime;
}

Time::Time(uint64_t currentTime) : timeNow(currentTime), timeBefore(), deltaTime() {}

void Time::Update()
{
	this->timeBefore = this->timeNow;
	this->timeNow = SDL_GetPerformanceCounter();
	this->deltaTime = ((this->timeNow - this->timeBefore)) / (double)SDL_GetPerformanceFrequency();
}