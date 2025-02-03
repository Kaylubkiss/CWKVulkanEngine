#pragma once
#include <thread>
#include <functional>
#include <queue>
#include <condition_variable>

class ThreadPool 
{
public:
	ThreadPool(size_t num_threads = std::thread::hardware_concurrency() * .5f);
	void EnqueueTask(const std::function<void()>& task);
	void Terminate();
	bool isBusy();
	~ThreadPool();


private:
	void ThreadLoop();
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	std::mutex queue_mutex; //to make sure that the job queue is not corrupted.
	std::condition_variable condition_variable;
	bool terminate = false;
};