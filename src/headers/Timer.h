#pragma once

#include <chrono>

class Timer 
{
public:
	Timer();
	double CalculateDeltaTime();
	[[nodiscard]] double GetFPS() const;
private:
	std::chrono::steady_clock::time_point timeNow;
	std::chrono::steady_clock::time_point timeBefore;

	double deltaTime = 0;
	double fps = 0;
	double elapsedSecond = 0;
	int deltaCount = 0;
};