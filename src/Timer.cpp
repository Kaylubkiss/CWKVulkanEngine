#include "Timer.h"

double Timer::DeltaTime() const
{
	return this->deltaTime;
}

Timer::Timer(uint64_t currentTime) : timeNow(currentTime), timeBefore(), deltaTime() {}

void Timer::Update()
{
	this->timeBefore = this->timeNow;
	this->timeNow = SDL_GetPerformanceCounter();
	this->deltaTime = ((this->timeNow - this->timeBefore)) / (double)SDL_GetPerformanceFrequency();
}