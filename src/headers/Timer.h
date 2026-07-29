#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

class Timer 
{
public:
	Timer();
	double CalculateDeltaTime();
	[[nodiscard]] double GetFPS() const;
	[[nodiscard]] double GetDeltaTime() const;
private:
	std::chrono::steady_clock::time_point timeNow;
	std::chrono::steady_clock::time_point timeBefore;

	double deltaTime = 0;
	double fps = 0;
	double elapsedSecond = 0;
	int deltaCount = 0;
};

#endif