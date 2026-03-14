#include "Timer.h"

Timer::Timer()
{
	timeNow = std::chrono::high_resolution_clock::now();
}

double Timer::GetFPS() const
{
	return fps;
}

double Timer::CalculateDeltaTime()
{
	this->timeBefore = this->timeNow;
	this->timeNow = std::chrono::high_resolution_clock::now();

	deltaTime = std::chrono::duration<double, std::milli>(timeNow - timeBefore).count() / 1000.0;

	if (elapsedSecond >= 1.0)
	{
		fps = deltaCount;
		deltaCount = 0;
		elapsedSecond = 0;
	}

	elapsedSecond += deltaTime;
	++deltaCount;

	return deltaTime;
}